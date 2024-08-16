#pragma once
#include "geo.h"

#include <string>
#include <vector>

namespace catalog {

struct Stop {
    std::string name;
    geo::Coordinates coordinate;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

} // namespace catalog

template<>
struct std::less<const catalog::Stop*> {
    bool operator()(const catalog::Stop* lhs, const catalog::Stop* rhs) const {
        return lhs->name < rhs->name;
    }
};

template<>
struct std::less<const catalog::Bus*> {
    bool operator()(const catalog::Bus* lhs, const catalog::Bus* rhs) const {
        return lhs->name < rhs->name;
    }
};
