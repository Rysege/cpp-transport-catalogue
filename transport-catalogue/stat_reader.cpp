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
    using namespace std::string_literals;
    if (response.count_stops == 0) {
        return os << "not found"s;
    }

    return os << response.count_stops << " stops on route, "s
        << response.count_uniq_stops << " unique stops, "s
        << std::setprecision(6) << response.route_length << " route length"s;
}

std::ostream& operator<<(std::ostream& os, const response::BusForStop& response) {
    using namespace std::string_literals;
    if (!response) {
        return os << "not found"s;
    }

    if (response.value() == nullptr) {
        return os << "no buses"s;
    }

    os << "buses"s;
    for (auto& bus : *response.value()) {
        os << " "s << bus;
    }
    return os;
}

void RequestCatalogue(std::istream& in, std::ostream& out, const TransportCatalogue& catalogue) {
    int stat_request_count;
    in >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        std::getline(in, line);
        ParseAndPrint(catalogue, line, out);
    }
}

void ParseAndPrint(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    using namespace std::string_literals;

    const auto query = detail::ParseRequest(request);
    output << query.type << " "s << query.text << ": "s;

    if (query.type == "Bus"s) {
        output << transport_catalogue.GetBusInfo(query.text);
    }
    else if (query.type == "Stop"s) {
        output << transport_catalogue.GetStopInfo(query.text);
    }

    output << std::endl;
}
} // namespase output
} // namespace catalog