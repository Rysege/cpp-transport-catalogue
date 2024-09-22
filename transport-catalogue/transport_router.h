#include "router.h"
#include "transport_catalogue.h"

namespace routemap {

struct  RoutingSetting {
    double bus_velocity;
    int bus_wait_time;
};

struct Way {
    std::string_view name;
    int span_count;
    double time;
};

class TransportRouter {
    using Graph = graph::DirectedWeightedGraph<double>;

public:
    TransportRouter() = default;

    void operator()(RoutingSetting setting) {
        setting_ = std::move(setting);
    }


    void BuildGraph(const catalog::TransportCatalogue& db);
    std::optional<std::pair<double, std::vector<Way>>> FindBestRoute(std::string_view from, std::string_view to) const;

    operator bool() {
        return router_.has_value();
    }

    void Clear() {
        router_ = std::nullopt;
        graph_ = std::nullopt;
    }

private:
    RoutingSetting setting_;
    std::unordered_map<std::string_view, graph::VertexId> dict_vertices_;
    std::unordered_map<graph::EdgeId, Way> items_;
    std::optional<graph::Router<double>> router_;
    std::optional<Graph> graph_;

    std::pair<bool, graph::VertexId> AssignVertexId(std::string_view name);
    double ComputeTravelTime(int dist) const;
    void AddEdge(graph::VertexId id1, graph::VertexId id2, Way);
    std::optional<graph::VertexId> GetVertexId(std::string_view name) const;
};

} // namespace routemap