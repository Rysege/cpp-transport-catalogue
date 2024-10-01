#pragma once
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <deque>

namespace handler {

class BaseQuery {
public:
    virtual ~BaseQuery() = default;
    virtual void Process(catalog::TransportCatalogue& db) const = 0;
};

struct StatRequest {
    std::unordered_map<std::string_view, std::string_view> params;
    int id = 0;
};

// вспомогательный класс для обработки BaseRequest
class BaseQueryHandler {
public:
    friend class RequestHandler;

    template<typename Tuple>
    void AddBaseQuery(const Tuple& params) {
        const auto& [param1, param2, param3] = params;
        Add(param1, param2, param3);
    }

protected:
    void ProcessBaseQuery(catalog::TransportCatalogue& tc) const;
    void Add(std::string_view name,
             geo::Coordinates coordinates,
             std::unordered_map<std::string_view, int> road_distances);

    void Add(std::string_view name, 
             std::vector<std::string_view> stops,
             bool is_roundtrip);

private:
    std::deque<std::unique_ptr<BaseQuery>> base_queries_;
};

class RequestHandler {
public:
    RequestHandler(catalog::TransportCatalogue& catalogue)
        : db_(catalogue) {
    }

    void ProcessBaseQuery(const BaseQueryHandler& handler) const;
    std::vector<json::Node> ProcessStatQuery(const std::vector<StatRequest>& requests,
                                              renderer::RenderSettings render_settings,
                                              routemap::RoutingSettings routing_settings) const;

private:
    catalog::TransportCatalogue& db_;
};

} // namespace handler
