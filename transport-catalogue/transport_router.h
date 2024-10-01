#include "router.h"
#include "transport_catalogue.h"

namespace routemap {

struct  RoutingSettings {
    double bus_velocity;
    int bus_wait_time;
};

struct Way {
    std::string_view name;
    int span_count{};
    double time{};
};

struct FoundRoute {
    double total_time;
    std::vector<Way> ways;
};

class TransportRouter {
public:
    TransportRouter(RoutingSettings setting, const catalog::TransportCatalogue& db);

    std::optional<FoundRoute> FindBestRoute(std::string_view from, std::string_view to) const;

private:
    RoutingSettings settings_;
    std::unordered_map<std::string_view, graph::VertexId> dict_vertices_;
    std::unordered_map<graph::EdgeId, Way> items_;
    std::optional<graph::Router<double>> router_;
    graph::DirectedWeightedGraph<double> graph_;

    std::pair<bool, graph::VertexId> AssignVertexId(std::string_view name);
    std::optional<graph::VertexId> GetVertexId(std::string_view name) const;
    double ComputeTravelTime(int dist) const;
    void AddEdge(graph::VertexId id1, graph::VertexId id2, Way);
    void BuildGraph(const catalog::TransportCatalogue& db);
};

} // namespace routemap