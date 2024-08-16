#pragma once
#include "domain.h"

#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace catalog {

struct BusStat {
    int count_stops = 0;
    int count_uniq_stops = 0;
    double route_length = 0.0;
    double curvature = 0.0;
};

using BusesByStop = std::optional<const std::set<std::string_view>*>;

struct Hasher {
    size_t operator()(const std::pair<const Stop*, const Stop*>& pair_stops) const {
        return hasher(pair_stops.first->name) + hasher(pair_stops.second->name) * 37;
    }

private:
    std::hash<std::string_view> hasher;
};

class TransportCatalogue {
public:
    void AddBus(std::string_view bus_name, const std::vector<std::string_view>& stops, bool is_roundtrip);
    void AddStop(std::string_view stop_name, const geo::Coordinates coordinates);
    void SetDistanceBetweenStops(std::string_view from_stop, std::string_view to_stop, const int dist);
    const std::set<const Bus*> GetRoutes() const;
    const Stop* GetStop(std::string_view stop_name) const;

    BusStat GetBusStat(std::string_view bus_name) const;
    BusesByStop GetBusesByStop(std::string_view stop_name) const;

private:
    std::unordered_map<std::string_view, std::unique_ptr<Stop>> stops_;
    std::unordered_map<std::string_view, std::unique_ptr<Bus>> buses_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stops_to_buses_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, Hasher> stop_distance_;

    double GetDistanceBetweenStops(const Stop* from, const Stop* to) const;

    template <typename Iterator>
    double CalcDistanceRoute(Iterator begin, Iterator end) const {
        return std::transform_reduce(std::next(begin), end, begin, 0.0, std::plus(),
            [&](auto rhs, auto lhs) { return GetDistanceBetweenStops(lhs, rhs); });
    }
};

template<typename Iterator>
double CalcDistanceRouteGeo(Iterator begin, Iterator end) {
    return std::transform_reduce(std::next(begin), end, begin, 0.0, std::plus(),
        [](auto lhs, auto rhs) { return geo::ComputeDistance(lhs->coordinate, rhs->coordinate); });
}

} // namespace catalog
