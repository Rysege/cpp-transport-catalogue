#include "request_handler.h"

namespace handler {

using namespace catalog;
using namespace json;
using namespace renderer;
using namespace routemap;
using namespace std::literals;

class StatQuery {
public:
    StatQuery(int id) : id_(id) {}
    virtual ~StatQuery() = default;
    virtual Node Process(const TransportCatalogue&, const MapRenderer&, TransportRouter&) const = 0;

protected:
    int GetId() const {
        return id_;
    }

    Node Build(std::string str = "not found") const {
        return Builder{}.StartDict()
            .Key("request_id"s).Value(id_)
            .Key("error_message"s).Value(std::move(str))
            .EndDict().Build();
    }

private:
    int id_;
};

class StatQueryFactory {
public:
    virtual ~StatQueryFactory() = default;
    virtual std::unique_ptr<StatQuery> Create(const StatRequest& config) const {
        return GetFactory(config.params.at("type")).Create(config);
    }

private:
    static const StatQueryFactory& GetFactory(std::string_view type);
};

void RequestHandler::ProcessBaseQuery(const BaseQueryHandler& handler) const {
    router_.Clear();
    handler.ProcessBaseQuery(db_);
}

Node RequestHandler::ProcessStatQuery(const StatRequest& config) const {
    const StatQueryFactory factory;
    return factory.Create(config)->Process(db_, renderer_, router_);
}

void RequestHandler::SetSettingMapRenderer(renderer::RenderSetting setting) {
    renderer_(setting);
}

void RequestHandler::SetSettingTransportRouter(routemap::RoutingSetting setting) {
    router_(setting);
}

namespace base_queries {

// добавляет остановки в каталог
class QueryStop : public BaseQuery {
public:
    QueryStop(std::string_view name, geo::Coordinates coordinates)
        : name_(name)
        , coordinates_(coordinates) {
    }

    void Process(catalog::TransportCatalogue& db) const override {
        db.AddStop(name_, coordinates_);
    }

private:
    std::string_view name_;
    geo::Coordinates coordinates_;
};

// добавляет дистанции между остановками в каталог
class QueryStopDistance : public BaseQuery {
public:
    QueryStopDistance(std::string_view name, std::unordered_map<std::string_view, int> road_distances)
        : name_(name)
        , road_distances_(std::move(road_distances)) {
    }

    void Process(TransportCatalogue& db) const override {
        for (auto& [to, dist] : road_distances_) {
            db.SetDistance(name_, to, dist);
        }
    }

private:
    std::string_view name_;
    std::unordered_map<std::string_view, int> road_distances_;
};

// добавляет маршруты в каталог
class QueryBus : public BaseQuery {
public:
    QueryBus(std::string_view name, std::vector<std::string_view> stops, bool is_roundtrip)
        : name_(name)
        , stops_(std::move(stops))
        , is_roundtrip_(is_roundtrip) {
    }

    void Process(TransportCatalogue& db) const override {
        db.AddBus(name_, stops_, is_roundtrip_);
    }

private:
    std::string_view name_;
    std::vector<std::string_view> stops_;
    bool is_roundtrip_ = false;
};

} // namespace base_queries

namespace stat_queries {

class StatQueryStop : public StatQuery {
public:
    StatQueryStop(int id, std::string_view name)
        : StatQuery(id)
        , name_(name) {
    }

    Node Process(const TransportCatalogue& db, const MapRenderer&, TransportRouter&) const override {
        if (const auto& response = db.GetBusesByStop(name_)) {
            return Build(response.value());
        }
        return StatQuery::Build();
    }

    class Factory : public StatQueryFactory {
        std::unique_ptr<StatQuery> Create(const StatRequest& config) const override {
            return std::make_unique<StatQueryStop>(config.id, config.params.at("name"sv));
        }
    };
private:
    std::string_view name_;

    Node Build(const std::set<std::string_view>* buses) const {
        return Builder{}.StartDict()
            .Key("request_id"s).Value(GetId())
            .Key("buses"s).Value(BuildArray(buses))
            .EndDict().Build();
    }

    std::vector<Node> BuildArray(const std::set<std::string_view>* buses) const {
        std::vector<Node> result;
        if (buses) {
            for (std::string_view str : *buses) {
                result.push_back(Builder{}.Value(std::string(str)).Build());
            }
        }
        return result;
    };
};

class StatQueryBus : public StatQuery {
public:
    StatQueryBus(int id, std::string_view name)
        : StatQuery(id)
        , name_(name) {
    }

    Node Process(const TransportCatalogue& db, const MapRenderer&, TransportRouter&) const override {
        const auto& response = db.GetBusStat(name_);
        if (response.count_stops != 0) {
            return Build(response);
        }
        return StatQuery::Build();
    }

    class Factory : public StatQueryFactory {
        std::unique_ptr<StatQuery> Create(const StatRequest& config) const override {
            return std::make_unique<StatQueryBus>(config.id, config.params.at("name"sv));
        }
    };
private:
    std::string_view name_;

    Node Build(const catalog::BusStat& stat) const {
        return Builder{}.StartDict()
            .Key("request_id"s).Value(GetId())
            .Key("curvature"s).Value(stat.curvature)
            .Key("route_length"s).Value(stat.route_length)
            .Key("stop_count"s).Value(stat.count_stops)
            .Key("unique_stop_count"s).Value(stat.count_uniq_stops)
            .EndDict().Build();
    }
};

class StatQueryMap : public StatQuery {
public:
    using StatQuery::StatQuery;

    Node Process(const TransportCatalogue& db, const MapRenderer& renderer, TransportRouter&) const override {
        std::ostringstream os;
        renderer.RenderMap(db.GetRoutes()).Render(os);
        return Build(os.str());
    }

    class Factory : public StatQueryFactory {
        std::unique_ptr<StatQuery> Create(const StatRequest& config) const override {
            return std::make_unique<StatQueryMap>(config.id);
        }
    };
private:
    Node Build(std::string str) const {
        return Builder{}.StartDict()
            .Key("request_id"s).Value(GetId())
            .Key("map"s).Value(std::move(str))
            .EndDict().Build();
    }
};

class StatQueryRoute : public StatQuery {
public:
    StatQueryRoute(int id, std::string_view from, std::string_view to)
        : StatQuery(id)
        , from_(from)
        , to_(to) {
    }

    Node Process(const TransportCatalogue& db, const MapRenderer&, TransportRouter& router) const override {
        if (!router) {
            router.BuildGraph(db);
        }
        const auto result = router.FindBestRoute(from_, to_);
        if (result) {
            return Build(result->first, result->second);
        }
        return StatQuery::Build();
    }

    class Factory : public StatQueryFactory {
        std::unique_ptr<StatQuery> Create(const StatRequest& config) const override {
            return std::make_unique<StatQueryRoute>(config.id, config.params.at("from"sv), config.params.at("to"sv));
        }
    };
private:
    std::string_view from_;
    std::string_view to_;

    Node Build(double total_time, const std::vector<Way>& items) const {
        return Builder{}.StartDict()
            .Key("request_id"s).Value(GetId())
            .Key("total_time"s).Value(total_time)
            .Key("items"s).Value(std::move(BuildArray(items)))
            .EndDict().Build();
    }

    std::vector<Node> BuildArray(const std::vector<Way>& items) const {
        std::vector<Node> result;
        for (auto& item : items) {
            result.push_back(item.span_count ? BuildItemBus(item) : BuildItemWait(item));
        }
        return result;
    }

    Node BuildItemWait(const Way& item) const {
        return Builder{}.StartDict()
            .Key("type"s).Value("Wait"s)
            .Key("stop_name"s).Value(std::string(item.name))
            .Key("time").Value(item.time)
            .EndDict().Build();
    }

    Node BuildItemBus(const Way& item) const {
        return Builder{}.StartDict()
            .Key("type"s).Value("Bus"s)
            .Key("bus"s).Value(std::string(item.name))
            .Key("span_count"s).Value(item.span_count)
            .Key("time").Value(item.time)
            .EndDict().Build();
    }
};

} // namespace stat_queries


const StatQueryFactory& StatQueryFactory::GetFactory(std::string_view type) {
    static stat_queries::StatQueryStop::Factory stop;
    static stat_queries::StatQueryBus::Factory bus;
    static stat_queries::StatQueryMap::Factory map;
    static stat_queries::StatQueryRoute::Factory route;

    static std::unordered_map<std::string_view, const StatQueryFactory&> factories = {
        { "Stop"sv, stop },
        { "Bus"sv, bus },
        { "Map"sv, map },
        { "Route"sv, route }
    };
    return factories.at(type);
}

void BaseQueryHandler::AddBaseQuery(std::string_view name,
                                    geo::Coordinates coordinates,
                                    std::unordered_map<std::string_view, int> road_distances) {

    base_queries_.push_front(std::make_unique<base_queries::QueryStop>(name, coordinates));
    base_queries_.push_back(std::make_unique<base_queries::QueryStopDistance>(name, road_distances));
}

void BaseQueryHandler::AddBaseQuery(std::string_view name,
                                    std::vector<std::string_view> stops,
                                    bool is_roundtrip) {

    base_queries_.push_back(std::make_unique<base_queries::QueryBus>(name, stops, is_roundtrip));
}

void BaseQueryHandler::ProcessBaseQuery(TransportCatalogue& tc) const {
    for (const auto& query : base_queries_) {
        query->Process(tc);
    }
}

} // namespace handler