#include <iomanip>
#include <iostream>

#include "stat_reader.h"

namespace catalog {
namespace output {
namespace detail {

Query ParseRequest(std::string_view request) {
    const auto space_pos = request.find(' ');
    if (space_pos >= request.npos) {
        return {};
    }

    const auto not_space = request.find_first_not_of(' ', space_pos);
    if (not_space >= request.npos) {
        return {};
    }

    const auto last_not_space = request.find_last_not_of(' ') + 1;
    return { request.substr(0,space_pos),
        request.substr(not_space, last_not_space - not_space) };
}
} // namespace detail

std::ostream& operator<<(std::ostream& os, const response::RouteInfo& response) {
    using namespace std::literals;
    if (response.count_stops == 0) {
        return os << "not found"sv;
    }

    return os << response.count_stops << " stops on route, "sv
        << response.count_uniq_stops << " unique stops, "sv
        << std::setprecision(6) << response.route_length << " route length, "sv
        << response.curvature << " curvature"sv;
}

std::ostream& operator<<(std::ostream& os, const response::BusForStop& response) {
    using namespace std::literals;
    if (!response) {
        return os << "not found"sv;
    }

    if (response.value() == nullptr) {
        return os << "no buses"sv;
    }

    os << "buses"sv;
    for (auto& bus : *response.value()) {
        os << " "sv << bus;
    }
    return os;
}

void RequestCatalogue(const TransportCatalogue& catalogue, std::istream& in, std::ostream& out) {
    int stat_request_count;
    in >> stat_request_count;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        std::getline(in >> std::ws, line);
        ProcessRequest(catalogue, line, out);
    }
}

void ProcessRequest(const TransportCatalogue& catalogue, std::string_view request, std::ostream& out) {
    using namespace std::literals;

    const auto query = detail::ParseRequest(request);
    if (query.type.empty()) {
        return;
    }
    out << query.type << " "sv << query.text << ": "sv;

    if (query.type == "Bus"sv) {
        out << catalogue.GetBusInfo(query.text);
    }
    else if (query.type == "Stop"sv) {
        out << catalogue.GetStopInfo(query.text);
    }
    out << std::endl;
}
} // namespase output
} // namespace catalog