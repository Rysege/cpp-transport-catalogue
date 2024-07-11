#include <algorithm>
#include <cassert>
#include <numeric>
#include <utility>

#include "transport_catalogue.h"

namespace catalog {

void TransportCatalogue::AddBus(const std::string_view bus_name, const std::vector<std::string_view> stops) {
    std::vector<const Stop*> route;
    route.reserve(stops.size());
    for (auto& stop : stops) {
        assert(stopname_to_stop_.count(stop));
        route.emplace_back(stopname_to_stop_.at(stop));
    }

    buses_.push_back({ std::string(bus_name), std::move(route) });
    busname_to_bus_[buses_.back().name] = &buses_.back();

    std::string_view name_bus = buses_.back().name;
    for (auto& stop : buses_.back().stops) {
        stops_to_buses_[stop->name].emplace(name_bus);
    }
}

void TransportCatalogue::AddStop(const std::string_view stop_name, const geo::Coordinates coordinates) {
    assert(!stopname_to_stop_.count(stop_name));
    stops_.push_back({ std::string(stop_name), coordinates });
    stopname_to_stop_[stops_.back().name] = &stops_.back();
}

response::RouteInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    if (!busname_to_bus_.count(bus_name)) {
        return {};
    }

    response::RouteInfo route_info{};
    auto& route = busname_to_bus_.at(bus_name)->stops;
    route_info.count_stops = route.size();

    auto prev = route[0]->coordinate;
    std::unordered_set<std::string_view> uniq_stops{ route[0]->name };
    route_info.route_length = std::accumulate(std::next(route.begin()), route.end(), 0.0,
        [&](auto lenght, const auto& stop) {
            uniq_stops.insert(stop->name);
            return lenght + geo::ComputeDistance(std::exchange(prev,stop->coordinate), stop->coordinate);
        });

    route_info.count_uniq_stops = uniq_stops.size();

    return route_info;
}

response::BusForStop TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    if (!stopname_to_stop_.count(stop_name)) {
        return {};
    }

    if (!stops_to_buses_.count(stop_name)) {
        return { true, {} };
    }

    std::vector<std::string_view> buses;
    for (auto& bus : stops_to_buses_.at(stop_name)) {
        buses.emplace_back(bus);
    }

    std::sort(buses.begin(), buses.end());

    return { true, buses };
}
} // namespace catalog