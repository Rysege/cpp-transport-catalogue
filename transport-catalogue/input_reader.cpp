#include <cassert>
#include <istream>
#include <iterator>

#include "geo.h"
#include "input_reader.h"

namespace catalog {
namespace input {

void LoadCatalogue(TransportCatalogue& catalogue, std::istream& in) {
    int base_request_count;
    in >> base_request_count;

    InputReader reader;
    for (int i = 0; i < base_request_count; ++i) {
        std::string line;
        std::getline(in >> std::ws, line);
        reader.ParseLine(line);
    }
    reader.ApplyCommands(catalogue);
}

namespace detail {
/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}
} // namespace detail

namespace parse {
/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
geo::Coordinates Coordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return { nan, nan };
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return { lat, lng };
}

std::unordered_map<std::string_view, int> Distance(std::string_view str) {
    using namespace std::literals;
    size_t delim_pos = 0;
    std::unordered_map<std::string_view, int> result;
    while ((delim_pos = str.find("m to"sv, delim_pos)) != str.npos) {
        auto num_pos = str.rfind(' ', delim_pos) + 1;
        if (str[num_pos] < '0' || str[num_pos] > '9') {
            return {};
        }

        auto not_space = str.find_first_not_of(' ', delim_pos + 5);
        if (not_space == str.npos) {
            return{};
        }

        auto comma = std::min(str.find(',', not_space), str.size());
        int dist = std::stoi(std::string(str.substr(num_pos, delim_pos - num_pos)));
        result[str.substr(not_space, comma - not_space)] = dist;
        delim_pos = comma;
    }
    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> Route(std::string_view route) {
    if (route.find('>') != route.npos) {
        return detail::Split(route, '>');
    }

    auto stops = detail::Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription Command(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return { std::string(line.substr(0, space_pos)),
        std::string(line.substr(not_space, colon_pos - not_space)),
        std::string(line.substr(colon_pos + 1)) };
}
} // namespace parse

void InputReader::ParseLine(std::string_view line) {
    auto command_description = parse::Command(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands(TransportCatalogue& catalogue) const {
    using namespace std::literals;

    for (auto& append : commands_) {
        if (append.command == "Stop"sv) {
            catalogue.AddStop(append.id, parse::Coordinates(append.description));
        }
    }

    for (auto& append : commands_) {
        if (append.command == "Stop"sv) {
            for (auto& [to, dist] : parse::Distance(append.description)) {
                catalogue.SetDistanceBetweenStops(append.id, to, dist);
            }
        }

        if (append.command == "Bus"sv) {
            catalogue.AddBus(append.id, parse::Route(append.description));
        }
    }
}
} // namespace input
} // namespace catalog