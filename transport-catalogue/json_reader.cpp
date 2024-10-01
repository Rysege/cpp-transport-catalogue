#include "json_builder.h"
#include "json_reader.h"

#include <tuple>

namespace json_reader {

using namespace json;
using namespace handler;
using namespace std::literals;

namespace detail {

std::unordered_map<std::string_view, int> ConvertDict(const Dict& dict) {
    return ConvertTo<std::unordered_map<std::string_view, int>>(
        dict.begin(), dict.end(),
        [](const auto& p) { return std::pair<std::string_view, int>{ p.first, p.second.AsInt() }; });
}

std::vector<std::string_view> ConvertArray(const Array& arr) {
    return ConvertTo<std::vector<std::string_view>>(
        arr.begin(), arr.end(), [](const Node& stop) { return std::string_view{ stop.AsString() }; });
}

using ParamsQueryStop = std::tuple<std::string_view, geo::Coordinates, std::unordered_map<std::string_view, int>>;

ParamsQueryStop ParseStopRequest(const Dict& dict) {
    return{
        dict.at("name"s).AsString(),
        geo::Coordinates{ dict.at("latitude"s).AsDouble(), dict.at("longitude"s).AsDouble() },
        std::move(ConvertDict(dict.at("road_distances"s).AsDict()))
    };
}

using ParamsQueryBus = std::tuple<std::string_view, std::vector<std::string_view>, bool>;

ParamsQueryBus ParseBusRequest(const Dict& dict) {
    return{
        dict.at("name"s).AsString(),
        std::move(ConvertArray(dict.at("stops"s).AsArray())),
        dict.at("is_roundtrip"s).AsBool()
    };
}

StatRequest ParseStatRequest(const Dict& dict) {
    StatRequest result;
    for (const auto& [key, val] : dict) {
        if (key == "id"sv) {
            result.id = val.AsInt();
        }
        else {
            result.params[key] = val.AsString();
        }
    }
    return result;
}

svg::Color ConvertToColor(const Node& node) {
    if (node.IsArray()) {
        const Array& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb(
                static_cast<uint8_t>(arr.at(0).AsInt()),
                static_cast<uint8_t>(arr.at(1).AsInt()),
                static_cast<uint8_t>(arr.at(2).AsInt()));
        }
        if (arr.size() == 4) {
            return svg::Rgba(
                static_cast<uint8_t>(arr.at(0).AsInt()),
                static_cast<uint8_t>(arr.at(1).AsInt()),
                static_cast<uint8_t>(arr.at(2).AsInt()),
                arr.at(3).AsDouble());
        }
    }
    return node.AsString();
}

svg::Point ConvertToPoint(const Array& arr) {
    if (arr.size() == 2) {
        return svg::Point{ arr.at(0).AsDouble(), arr.at(1).AsDouble() };
    }
    throw RequestError();
}

std::vector<svg::Color> ConvertToArrayColor(const Array& arr) {
    std::vector<svg::Color> result;
    std::transform(arr.begin(), arr.end(), std::back_inserter(result),
        [](const Node& node) {return  svg::Color(ConvertToColor(node)); });

    return result;
}

} // namespace detail

using namespace detail;

const Node& JsonReader::GetNodeRequest(const std::string& name) const {
    const Dict& requests = data_.GetRoot().AsDict();

    if (auto it = requests.find(name); it != requests.end()) {
        return it->second;
    }
    throw RequestError();
}

void JsonReader::LoadData() const {
    const Array& base_requests = GetNodeRequest("base_requests"s).AsArray();

    BaseQueryHandler queries;

    for (const auto& request : base_requests) {
        const Dict& dict = request.AsDict();
        std::string_view type = dict.at("type"s).AsString();
        if (type == "Stop"sv) {
            queries.AddBaseQuery(TryCatch(ParseStopRequest, dict));
        }
        if (type == "Bus"sv) {
            queries.AddBaseQuery(TryCatch(ParseBusRequest, dict));
        }
    }
    handler_.ProcessBaseQuery(queries);
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    const Dict& dict = GetNodeRequest("render_settings"s).AsDict();

    renderer::RenderSettings setting;
    try {
        setting.width = dict.at("width"s).AsDouble();
        setting.height = dict.at("height"s).AsDouble();
        setting.padding = dict.at("padding"s).AsDouble();
        setting.stop_radius = dict.at("stop_radius"s).AsDouble();
        setting.line_width = dict.at("line_width"s).AsDouble();
        setting.bus_label_font_size = dict.at("bus_label_font_size"s).AsInt();
        setting.bus_label_offset = ConvertToPoint(dict.at("bus_label_offset"s).AsArray());
        setting.stop_label_font_size = dict.at("stop_label_font_size"s).AsInt();
        setting.stop_label_offset = ConvertToPoint(dict.at("stop_label_offset"s).AsArray());
        setting.underlayer_color = ConvertToColor(dict.at("underlayer_color"s));
        setting.underlayer_width = dict.at("underlayer_width"s).AsDouble();
        setting.color_palette = std::move(ConvertToArrayColor(dict.at("color_palette"s).AsArray()));
    }
    catch (std::out_of_range const&) {
        throw RequestError("Invalid renderer settings");
    }
    catch (...) {
        throw;
    }
    return setting;
}

routemap::RoutingSettings JsonReader::ParseRoutingSettings() const {
    const Dict& dict = GetNodeRequest("routing_settings"s).AsDict();

    routemap::RoutingSettings settings{};
    try {
        settings.bus_wait_time = dict.at("bus_wait_time"s).AsInt();
        settings.bus_velocity = dict.at("bus_velocity"s).AsDouble();
    }
    catch (std::out_of_range const&) {
        throw RequestError("Invalid renderer settings");
    }
    catch (...) {
        throw;
    }
    return settings;
}

Document JsonReader::ProcessStatRequests() const {
    const Array& stat_requests = GetNodeRequest("stat_requests"s).AsArray();

    std::vector<StatRequest> requests;
    for (const auto& request : stat_requests) {
        requests.push_back(ParseStatRequest(request.AsDict()));
    }
    return Document(handler_.ProcessStatQuery(requests, ParseRenderSettings(), ParseRoutingSettings()));
}

JsonReader::JsonReader(catalog::TransportCatalogue& db, std::istream& in)
    : data_(json::Load(in))
    , handler_(db) {
    LoadData();
}

void JsonReader::PrintStatRequest(std::ostream& out) const {
    Print(ProcessStatRequests(), out);
}

} // namespace json_reader
