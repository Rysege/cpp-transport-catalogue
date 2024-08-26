#pragma once
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace handler {

struct RequestStop {
    std::string_view name;
    geo::Coordinates coordinates;
    std::unordered_map<std::string_view, int> road_distances;
};

struct RequestBus {
    std::string_view name;
    std::vector<std::string_view> stops;
    bool is_roundtrip = false;
};

struct StatRequest {
    std::string_view type;
    std::string_view name;
    int id = 0;
};

struct StringMsg {
    std::string title;
    std::string msg;
};

using RequestTC = std::variant<RequestStop, RequestBus>;
using ResponseTC = std::variant<StringMsg, const std::set<std::string_view>*, catalog::BusStat>;

class RequestHandler {
public:
    RequestHandler(catalog::TransportCatalogue& catalogue, renderer::MapRenderer& renderer)
        : db_(catalogue)
        , renderer_(renderer) {
    }

    void PerformBaseRequest(const std::vector<RequestTC>& requests) const;
    ResponseTC PerformStatRequest(const StatRequest& request) const;
    void SetSettingMapRenderer(renderer::RenderSetting& setting);
    svg::Document RenderMap() const;

private:
    catalog::TransportCatalogue& db_;
    renderer::MapRenderer& renderer_;
};

} // namespace handler
