#pragma once
#include <cmath>

namespace geo {

struct Coordinates {
    double lat = 0.0;
    double lng = 0.0;
    bool operator==(const Coordinates& other) const {
        const double epsilon = 1e-6;
        return std::abs(lat - other.lat) < epsilon && std::abs(lng - other.lng) < epsilon;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

double ComputeDistance(Coordinates from, Coordinates to);

} // namespace geo
