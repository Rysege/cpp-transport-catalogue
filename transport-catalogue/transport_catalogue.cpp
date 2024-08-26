#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace catalog {

void TransportCatalogue::AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops, bool is_roundtrip) {
    if (bus_name.empty() || stops.size() < 2) {
        return;
    }
    std::vector<const Stop*> route;
    route.reserve(stops.size());

    for (auto stop : stops) {
        const auto it = stops_.find(stop);
        assert(it != stops_.end());
        route.push_back(it->second.get());
    }

    auto bus = std::make_unique<Bus>(Bus{ std::string(bus_name), std::move(route), is_roundtrip });
    auto bus_ptr = bus.get();
    buses_.insert({ bus_ptr->name, std::move(bus)});

    std::string_view name_bus = bus_ptr->name;
    for (auto stop : bus_ptr->stops) {
        stops_to_buses_[stop->name].insert(name_bus);
    }
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates coordinates) {
    if (stop_name.empty()) {
        return;
    }
    assert(!stops_.count(stop_name));
    auto stop = std::make_unique<Stop>(Stop{ std::string(stop_name), coordinates });
    stops_.insert({ stop.get()->name, std::move(stop)});
}

void TransportCatalogue::SetDistanceBetweenStops(std::string_view from_stop, std::string_view to_stop, const int dist) {
    if (from_stop.empty() || to_stop.empty() || dist <= 0) {
        return;
    }
    auto from = GetStop(from_stop);
    auto to = GetStop(to_stop);

    stop_distance_[{from, to}] = dist;

    auto it = stop_distance_.find({ to, from });
    if (it == stop_distance_.end()) {
        stop_distance_[{to, from}] = dist;
    }
}

const std::set<const Bus*> TransportCatalogue::GetRoutes() const{
    std::set<const Bus*> result;
    std::transform(buses_.begin(), buses_.end(), std::inserter(result, result.end()),
        [](const auto& bus) { return  bus.second.get(); });

    return result;
}

int GetCountUniqueStop(const std::vector<const Stop*>& route) {
    std::vector<const Stop*> tmp;
    for (auto stop : route) {
        if (find(tmp.begin(), tmp.end(), stop) == tmp.end()) {
            tmp.push_back(stop);
        }
    }
    return tmp.size();
}

BusStat TransportCatalogue::GetBusStat(std::string_view bus_name) const {
    const auto bus_iter = buses_.find(bus_name);
    if (bus_iter == buses_.end()) {
        return {};
    }

    const auto& route = bus_iter->second->stops;
    bool is_roundtrip = bus_iter->second->is_roundtrip;

    BusStat bus_stat;
    bus_stat.count_stops = is_roundtrip ? route.size(): route.size() * 2 - 1;
    bus_stat.count_uniq_stops = GetCountUniqueStop(route);
    bus_stat.route_length = CalcDistanceRoute(route.begin(), route.end());
    double dist_geo = CalcDistanceRouteGeo(route.begin(), route.end());

    if (!bus_iter->second->is_roundtrip) {
        bus_stat.route_length += CalcDistanceRoute(route.rbegin(), route.rend());
        dist_geo += CalcDistanceRouteGeo(route.rbegin(), route.rend());
    }
    bus_stat.curvature = bus_stat.route_length / dist_geo;

    return bus_stat;
}

BusesByStop TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
    if (!stops_.count(stop_name)) {
        return {};
    }
    const auto stop_iter = stops_to_buses_.find(stop_name);
    return stop_iter != stops_to_buses_.end() ? &stop_iter->second : nullptr;
}

double TransportCatalogue::GetDistanceBetweenStops(const Stop* from, const Stop* to) const {
    auto it = stop_distance_.find({ from, to });
    return it == stop_distance_.end() ? geo::ComputeDistance(from->coordinate, to->coordinate) : it->second;
}

const Stop* TransportCatalogue::GetStop(std::string_view stop_name) const {
    auto it = stops_.find(stop_name);
    assert(it != stops_.end());
    return  it->second.get();
}

} // namespace catalog