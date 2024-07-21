#include <cassert>
#include <numeric>
#include <unordered_set>
#include <utility>

#include "transport_catalogue.h"

namespace catalog {

void TransportCatalogue::AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops) {
    if (bus_name.empty() || stops.size() < 2) {
        return;
    }
    std::vector<const Stop*> route;
    route.reserve(stops.size());
    for (auto stop : stops) {
        const auto it = stopname_to_stop_.find(stop);
        assert(it != stopname_to_stop_.end());
        route.push_back(it->second);
    }

    buses_.push_back({ std::string(bus_name), std::move(route) });
    busname_to_bus_.insert({ buses_.back().name, &buses_.back() });

    std::string_view name_bus = buses_.back().name;
    for (auto stop : buses_.back().stops) {
        stops_to_buses_[stop->name].insert(name_bus);
    }
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates coordinates) {
    if (stop_name.empty()) {
        return;
    }
    assert(!stopname_to_stop_.count(stop_name));
    stops_.push_back({ std::string(stop_name), coordinates });
    stopname_to_stop_.insert({ stops_.back().name, &stops_.back() });
}

void TransportCatalogue::SetDistanceBetweenStops(std::string_view from_stop, std::string_view to_stop, const int dist) {
    if (from_stop.empty() || to_stop.empty() || dist <= 0) {
        return;
    }
    auto from = GetStop(from_stop);
    auto to = GetStop(to_stop);

    stop_distance_[{from, to}] = dist;

    auto it = stop_distance_.find({to, from});
    if (it == stop_distance_.end()) {
        stop_distance_[{to, from}] = dist;
    }
}

double CalcDistanceRouteGeo(const std::vector<const Stop*>& route)  {
    return std::transform_reduce(std::next(route.begin()), route.end(), route.begin(), 0.0, std::plus(),
        [](auto lhs, auto rhs) { return geo::ComputeDistance(lhs->coordinate, rhs->coordinate); });
}

double TransportCatalogue::CalcDistanceRoute(const std::vector<const Stop*>& route) const{
    return std::transform_reduce(std::next(route.begin()), route.end(), route.begin(), 0.0, std::plus(),
        [&](auto rhs, auto lhs) { return GetDistanceBetweenStops(lhs, rhs); });
}

int GetCountUniqueStop(const std::vector<const Stop*>& route) {
    std::unordered_set<const Stop*> uniq_stops{ route.begin(), route.end() };
    return uniq_stops.size();
}

response::RouteInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const auto bus_iter = busname_to_bus_.find(bus_name);
    if (bus_iter == busname_to_bus_.end()) {
        return {};
    }

    response::RouteInfo route_info{};
    const auto& route = bus_iter->second->stops;

    route_info.count_stops = route.size();
    route_info.count_uniq_stops = GetCountUniqueStop(route);
    route_info.route_length = CalcDistanceRoute(route);
    route_info.curvature = route_info.route_length / CalcDistanceRouteGeo(route);

    return route_info;
}

response::BusForStop TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    if (!stopname_to_stop_.count(stop_name)) {
        return {};
    }

    const auto stop_iter = stops_to_buses_.find(stop_name);
    if (stop_iter == stops_to_buses_.end()) {
        return { nullptr };
    }

    return { &stop_iter->second };
}

double TransportCatalogue::GetDistanceBetweenStops(const Stop* from, const Stop* to) const {
    auto it = stop_distance_.find({ from, to });
    if (it == stop_distance_.end()) {
        return geo::ComputeDistance(from->coordinate, to->coordinate);
    }
    return it->second;
}

const Stop* TransportCatalogue::GetStop(std::string_view stop_name) const {
    auto it = stopname_to_stop_.find(stop_name);
    assert(it != stopname_to_stop_.end());

    return  it->second;
}
} // namespace catalog