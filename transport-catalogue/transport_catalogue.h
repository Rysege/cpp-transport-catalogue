#pragma once
#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace catalog {
namespace response {

struct RouteInfo {
    int count_stops = 0;
    int count_uniq_stops = 0;
    double route_length = 0.0;
};

using BusForStop = std::optional<const std::set<std::string_view>*>;
} // namespace response

struct Stop {
    std::string name;
    geo::Coordinates coordinate;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
};

class TransportCatalogue {
public:
    void AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops);

    void AddStop(std::string_view stop_name, const geo::Coordinates coordinates);

    response::RouteInfo GetBusInfo(std::string_view bus_name) const;

    response::BusForStop GetStopInfo(std::string_view stop_name) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
};
} // namespace catalog