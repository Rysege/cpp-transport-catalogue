#include "json_reader.h"

namespace json_reader {

using namespace json;
using namespace handler;
using namespace std::literals;

namespace detail {

std::unordered_map<std::string_view, int> ConvertDict(const Dict& dict) {
    return ConvertTo<std::unordered_map<std::string_view, int>>(
        dict, [](const auto& p) { return std::pair<std::string_view, int>{ p.first, p.second.AsInt() }; });
}

std::vector<std::string_view> ConvertArray(const Array& arr) {
    return ConvertTo<std::vector<std::string_view>>(
        arr, [](const auto& stop) { return  std::string_view{ stop.AsString() }; });
}

RequestStop ParseStopRequest(const Dict& dict) {
    RequestStop request;
    request.name = dict.at("name"s).AsString();
    request.coordinates = { dict.at("latitude"s).AsDouble(), dict.at("longitude"s).AsDouble() };
    request.road_distances = std::move(ConvertDict(dict.at("road_distances"s).AsMap()));

    return request;
}

RequestBus ParseBusRequest(const Dict& dict) {
    RequestBus request;
    request.name = dict.at("name"s).AsString();
    request.stops = std::move(ConvertArray(dict.at("stops"s).AsArray()));
    request.is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    return request;
}

StatRequest ParseStatRequest(const Dict& dict) {
    StatRequest request;
    try {
        request.id = dict.at("id"s).AsInt();
        request.type = dict.at("type"s).AsString();
        if (dict.size() != 2) {
            request.name = dict.at("name"s).AsString();
        }
    }
    catch (std::out_of_range const&) {
        throw RequestError();
    }
    catch (...) {
        throw;
    }
    return request;
}

Node ConvertToArray(const std::set<std::string_view>* arr) {
    Array result;
    if (arr) {
        for (auto str : *arr) {
            result.push_back(std::move(Node(std::string(str))));
        }
    }
    return Node(std::move(result));
}

Node ConvertResponse(int id, const StringMsg& str) {
    return Node(Dict{
        { "request_id"s, Node(id) },
        { str.title, std::move(Node(str.msg)) } });
}

Node ConvertResponse(int id, const std::set<std::string_view>* arr) {
    return Node(Dict{
        { "request_id"s, Node(id) },
        { "buses"s, std::move(ConvertToArray(arr)) } });
}

Node ConvertResponse(int id, const catalog::BusStat& stat) {
    return Node(Dict{
        { "request_id"s, Node(id) },
        { "curvature"s, Node(stat.curvature) },
        { "route_length"s, Node(stat.route_length) },
        { "stop_count"s, Node(stat.count_stops) },
        { "unique_stop_count"s, Node(stat.count_uniq_stops) } });
}

Node ProcessResponse(int id, const ResponseTC& response) {
    return std::visit(
        [id](const auto& value) { return ConvertResponse(id, value); },
        response);
}

svg::Color ConvertToColor(const Node& node) {
    if (node.IsArray()) {
        auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb(
                arr.at(0).AsInt(),
                arr.at(1).AsInt(),
                arr.at(2).AsInt());
        }
        if (arr.size() == 4) {
            return svg::Rgba(
                arr.at(0).AsInt(),
                arr.at(1).AsInt(),
                arr.at(2).AsInt(),
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
        [](auto& node) {return  svg::Color(ConvertToColor(node)); });

    return result;
}

} // namespace detail

using namespace detail;

const Node& JsonReader::GetNodeRequest(const std::string& name) const {
    const auto& requests = data_.GetRoot().AsMap();

    if (auto it = requests.find(name); it != requests.end()) {
        return it->second;
    }
    throw RequestError();
}

void JsonReader::LoadDataToCatalogue(RequestHandler& handler) const {
    const auto& base_requests = GetNodeRequest("base_requests"s).AsArray();

    std::vector<RequestTC> requests;
    for (const auto& request : base_requests) {
        auto& as_map = request.AsMap();
        try {
            auto& type = as_map.at("type"s).AsString();
            if (type == "Stop"s) {
                requests.push_back(std::move(ParseStopRequest(as_map)));
            }
            if (type == "Bus"s) {
                requests.push_back(std::move(ParseBusRequest(as_map)));
            }
        }
        catch (std::out_of_range const&) {
            throw RequestError();
        }
        catch (...) {
            throw;
        }
    }
    handler.PerformBaseRequest(requests);
}

void JsonReader::LoadRenderSetting(handler::RequestHandler& handler) const {
    const auto& render_settings = GetNodeRequest("render_settings"s).AsMap();
    renderer::RenderSetting setting;
    try {
        setting.width = render_settings.at("width"s).AsDouble();
        setting.height = render_settings.at("height"s).AsDouble();
        setting.padding = render_settings.at("padding"s).AsDouble();
        setting.stop_radius = render_settings.at("stop_radius"s).AsDouble();
        setting.line_width = render_settings.at("line_width"s).AsDouble();
        setting.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
        setting.bus_label_offset = ConvertToPoint(render_settings.at("bus_label_offset"s).AsArray());
        setting.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
        setting.stop_label_offset = ConvertToPoint(render_settings.at("stop_label_offset"s).AsArray());
        setting.underlayer_color = ConvertToColor(render_settings.at("underlayer_color"s));
        setting.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
        setting.color_palette = std::move(ConvertToArrayColor(render_settings.at("color_palette"s).AsArray()));
    }
    catch (std::out_of_range const&) {
        throw RequestError("Invalid renderer settings");
    }
    catch (...) {
        throw;
    }
    handler.SetSettingMapRenderer(setting);
}

Document JsonReader::ProcessStatRequests(RequestHandler& handler) const {
    const auto& stat_requests = GetNodeRequest("stat_requests"s).AsArray();

    Array result;
    for (const auto& request : stat_requests) {
        const auto stat_request = ParseStatRequest(request.AsMap());
        const auto response = handler.PerformStatRequest(stat_request);
        result.push_back(std::move(ProcessResponse(stat_request.id, response)));
    }
    return Document(result);
}

void JsonReader::PrintStatRequest(handler::RequestHandler& handler, std::ostream& out) const {
    Print(ProcessStatRequests(handler), out);
}

void JsonReader::ReadJson(std::istream& in) {
    data_ = Load(in);
}

} // namespace json_reader
