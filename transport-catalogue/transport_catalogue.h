#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace catalog {
namespace response {

struct RouteInfo {
    int count_stops = 0;
    int count_uniq_stops = 0;
    double route_length = 0.0;
};

struct BusForStop {
    bool stop = false;
    std::vector<std::string_view> buses;
};
} // namespace response

class TransportCatalogue {
public:
    void AddBus(std::string_view bus_name, const std::vector<std::string_view> stops);

    void AddStop(std::string_view stop_name, const geo::Coordinates coordinates);

    response::RouteInfo GetBusInfo(std::string_view bus_name) const;

    response::BusForStop GetStopInfo(std::string_view stop_name) const;

private:
    struct Stop {
        std::string name;
        geo::Coordinates coordinate;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
    };

    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stops_to_buses_;
};
} // namespace catalog