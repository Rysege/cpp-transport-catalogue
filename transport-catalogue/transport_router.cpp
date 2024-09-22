#include "transport_router.h"

namespace routemap {

using namespace graph;
using namespace catalog;

double TransportRouter::ComputeTravelTime(int dist) const {
    static const double mpm = 60. / 1000;
    return dist / setting_.bus_velocity * mpm;
}

void TransportRouter::AddEdge(graph::VertexId id1, graph::VertexId id2, Way item) {
    EdgeId edge_id = graph_->AddEdge({ id1, id2, item.time });
    items_[edge_id] = std::move(item);
}

std::optional<graph::VertexId> TransportRouter::GetVertexId(std::string_view name) const {
    if (auto it = dict_vertices_.find(name); it != dict_vertices_.end()) {
        return it->second;
    }
    return {};
}

void TransportRouter::BuildGraph(const TransportCatalogue& db) {
    const auto& routes = db.GetRoutes();
    graph_.emplace((db.GetStopsCount() * 2));

    for (const Bus* route : routes) {
        const auto& stops = route->stops;

        for (int i = 0; i < stops.size() - 1; ++i) {
            auto [insertion, prev_vertex] = AssignVertexId(stops[i]->name);
            if (insertion) {
                AddEdge(prev_vertex, prev_vertex + 1, {  stops[i]->name, 0, setting_.bus_wait_time * 1. });
            }
            const Stop* prev_stop = stops[i];
            double travel_time = 0., travel_time_back = 0.;

            for (int j = i + 1; j < stops.size(); ++j) {
                if (stops[i] != stops[j]) {
                    auto [insertion, vertex] = AssignVertexId(stops[j]->name);
                    if (insertion) {
                        AddEdge(vertex, vertex + 1, { stops[j]->name, 0, setting_.bus_wait_time * 1. });
                    }
                    travel_time += ComputeTravelTime(db.GetDistance(prev_stop, stops[j]));
                    AddEdge(prev_vertex + 1, vertex, { route->name, j - i, travel_time });
                    if (!route->is_roundtrip) {
                        travel_time_back += ComputeTravelTime(db.GetDistance(stops[j], prev_stop));
                        AddEdge(vertex + 1, prev_vertex, { route->name, j - i, travel_time_back });
                    }
                }
                prev_stop = stops[j];
            }
        }
    }
    router_.emplace(Router<double>(*graph_));
}

std::optional<std::pair<double, std::vector<Way>>> TransportRouter::FindBestRoute(std::string_view from, std::string_view to) const {
    auto stop_from = GetVertexId(from);
    auto stop_to = GetVertexId(to);
    if (!stop_from || !stop_to) {
        return {};
    }

    const auto route = router_->BuildRoute(*stop_from, *stop_to);
    if (!route) {
        return {};
    }
    std::vector<Way> items;
    for (EdgeId id : route->edges) {
        items.push_back(items_.at(id));
    }
    return { { route->weight, items } };
}

std::pair<bool, VertexId> TransportRouter::AssignVertexId(std::string_view name) {
    auto [it, insertion] = dict_vertices_.emplace(name, dict_vertices_.size() * 2);
    return { insertion, it->second };
}

} // namespace transport_router
