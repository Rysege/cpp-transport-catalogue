#include <cassert>
#include <numeric>
#include <unordered_set>
#include <utility>

#include "transport_catalogue.h"

namespace catalog {

void TransportCatalogue::AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops) {
    std::vector<const Stop*> route;
    route.reserve(stops.size());
    for (auto& stop : stops) {
        const auto it = stopname_to_stop_.find(stop);
        assert(it != stopname_to_stop_.end());
        route.emplace_back(it->second);
    }

    buses_.push_back({ std::string(bus_name), std::move(route) });
    busname_to_bus_[buses_.back().name] = &buses_.back();

    std::string_view name_bus = buses_.back().name;
    for (auto& stop : buses_.back().stops) {
        stops_to_buses_[stop->name].emplace(name_bus);
    }
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates coordinates) {
    assert(!stopname_to_stop_.count(stop_name));
    stops_.push_back({ std::string(stop_name), coordinates });
    stopname_to_stop_[stops_.back().name] = &stops_.back();
}

response::RouteInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    const auto bus_iter = busname_to_bus_.find(bus_name);
    if (bus_iter == busname_to_bus_.end()) {
        return {};
    }

    response::RouteInfo route_info{};
    const auto& route = bus_iter->second->stops;
    route_info.count_stops = route.size();

    auto prev = route[0]->coordinate;
    std::unordered_set<std::string_view> uniq_stops{ route[0]->name };
    route_info.route_length = std::accumulate(std::next(route.begin()), route.end(), 0.0,
        [&](auto lenght, const auto& stop) {
            uniq_stops.insert(stop->name);
            return lenght + geo::ComputeDistance(std::exchange(prev, stop->coordinate), stop->coordinate);
        });

    route_info.count_uniq_stops = uniq_stops.size();

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
} // namespace catalog