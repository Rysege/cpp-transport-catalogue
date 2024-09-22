#pragma once
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <deque>

namespace handler {

class BaseQuery {
public:
    virtual ~BaseQuery() = default;
    virtual void Process(catalog::TransportCatalogue& db) const = 0;
};

struct StatRequest {
    std::unordered_map<std::string_view, std::string_view> params;
    int id = 0;
};

// вспомогательный класс для обработки BaseRequest
class BaseQueryHandler {
public:
    friend class RequestHandler;
    void AddBaseQuery(std::string_view name,
                      geo::Coordinates coordinates,
                      std::unordered_map<std::string_view, int> road_distances);

    void AddBaseQuery(std::string_view name, 
                      std::vector<std::string_view> stops,
                      bool is_roundtrip);

protected:
    void ProcessBaseQuery(catalog::TransportCatalogue& tc) const;

private:
    std::deque<std::unique_ptr<BaseQuery>> base_queries_;
};

class RequestHandler {
public:
    RequestHandler(catalog::TransportCatalogue& catalogue, routemap::TransportRouter& router, renderer::MapRenderer& renderer)
        : db_(catalogue)
        , router_(router)
        , renderer_(renderer) {
    }

    void ProcessBaseQuery(const BaseQueryHandler& handler) const;
    json::Node ProcessStatQuery(const StatRequest& request) const;
    void SetSettingMapRenderer(renderer::RenderSetting setting);
    void SetSettingTransportRouter(routemap::RoutingSetting setting);

private:
    catalog::TransportCatalogue& db_;
    routemap::TransportRouter& router_;
    renderer::MapRenderer& renderer_;

};

} // namespace handler
