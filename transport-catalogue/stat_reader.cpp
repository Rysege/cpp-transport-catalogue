#include <iomanip>

#include "stat_reader.h"

namespace catalog {
namespace output {
namespace detail {

Query ParseRequest(std::string_view request) {
    auto space_pos = request.find(' ');
    if (space_pos >= request.npos) {
        return {};
    }

    auto not_space = request.find_first_not_of(' ', space_pos);
    if (not_space >= request.npos) {
        return {};
    }

    return { std::string(request.substr(0,space_pos)),
        std::string(request.substr(not_space)) };
}

std::ostream& operator<<(std::ostream& os, const response::RouteInfo& response) {
    using namespace std::string_literals;
    os << ": "s;
    if (response.count_stops == 0) {
        return os << "not found"s;
    }

    return os << response.count_stops << " stops on route, "s
        << response.count_uniq_stops << " unique stops, "s
        << std::setprecision(6) << response.route_length << " route length"s;
}

std::ostream& operator<<(std::ostream& os, const response::BusForStop& response) {
    using namespace std::string_literals;
    os << ": "s;
    if (!response.stop) {
        return os << "not found"s;
    }

    if (response.buses.empty()) {
        return os << "no buses"s;
    }

    os << "buses"s;
    for (auto& bus : response.buses) {
        os << " "s << bus;
    }
    return os;
}
} // namespace detail

void ParseAndPrint(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    const auto query = detail::ParseRequest(request);
    output << request;

    using detail::operator<<;

    if (query.type == "Bus") {
        output << transport_catalogue.GetBusInfo(query.text);
    }
    else if (query.type == "Stop") {
        output << transport_catalogue.GetStopInfo(query.text);
    }

    output << std::endl;
}
} // namespase output
} // namespace catalog