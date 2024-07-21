#pragma once
#include <cmath>

namespace catalog {
namespace geo {

struct Coordinates {
    double lat;
    double lng;
    bool operator==(const Coordinates& other) const {
        const double epsilon = 1e-6;
        return std::abs(lat - other.lat) < epsilon && std::abs(lng - other.lng) < epsilon;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    const int r_earth = 6371000;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
        + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr)) * r_earth;
}
} // namespace geo
} // namespace catalog