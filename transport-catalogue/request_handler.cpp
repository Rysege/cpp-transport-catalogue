#include "request_handler.h"

namespace handler {

void RequestHandler::PerformBaseRequest(const std::vector<RequestTC>& requests) const {
    for (auto& request : requests) {
        if (const auto* stop = std::get_if<RequestStop>(&request)) {
            db_.AddStop(stop->name, stop->coordinates);
        }
    }

    for (auto& request : requests) {
        if (const auto* stop = std::get_if<RequestStop>(&request)) {
            for (auto& [to, dist] : stop->road_distances) {
                db_.SetDistanceBetweenStops(stop->name, to, dist);
            }
        }
        else if (const auto* bus = std::get_if<RequestBus>(&request)) {
            db_.AddBus(bus->name, bus->stops, bus->is_roundtrip);
        }
    }
}

ResponseTC RequestHandler::PerformStatRequest(const StatRequest& request) const {
    using namespace std::literals;
    if (request.type == "Stop"sv) {
        if (const auto& response = db_.GetBusesByStop(request.name)) {
            return response.value();
        }
    }
    else if (request.type == "Bus"sv) {
        const auto& response = db_.GetBusStat(request.name);
        if (response.count_stops != 0) {
            return response;
        }
    }
    else if (request.type == "Map"sv) {
        std::ostringstream os;
        RenderMap().Render(os);
        return StringMsg{ "map"s, os.str() };

    }
    return StringMsg{ "error_message"s, "not found"s };
}

void RequestHandler::SetSettingMapRenderer(renderer::RenderSetting& setting) {
    renderer_(setting);
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.RenderMap(db_.GetRoutes());
}

} // namespace handler