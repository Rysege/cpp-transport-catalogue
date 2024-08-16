#include "map_renderer.h"

namespace renderer {

using namespace svg;
using namespace std::literals;
using namespace catalog;

namespace detail {

void CreateUnderlay(svg::Text& text, const RenderSetting& setting) {
    text
        .SetFillColor(setting.underlayer_color)
        .SetStrokeColor(setting.underlayer_color)
        .SetStrokeWidth(setting.underlayer_width)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND);
}

std::vector<geo::Coordinates> GetCoordinatesOfBusStops(const std::set<const catalog::Bus*>& routes) {
    return ExtractData<std::vector<geo::Coordinates>>(
        routes, [](const catalog::Stop* stop) { return stop->coordinate; });
}

std::set<const catalog::Stop*> GetStopsFromRoutes(const std::set<const catalog::Bus*>& routes) {
    return ExtractData<std::set<const catalog::Stop*>>(
        routes, [](const catalog::Stop* stop) { return stop; });
}

} // namespace detail


bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

using namespace detail;

Document MapRenderer::RenderMap(const std::set<const Bus*>& routes) const {
    using namespace detail;

    const auto geo_coords = GetCoordinatesOfBusStops(routes);

    const SphereProjector proj{
        geo_coords.begin(), geo_coords.end(), setting_.width, setting_.height, setting_.padding
    };

    Document doc;
    RenderRoute(routes, proj, doc);
    RenderRouteName(routes, proj, doc);

    const auto stops = GetStopsFromRoutes(routes);
    RenderStop(stops, proj, doc);
    RenderStopName(stops, proj, doc);

    return doc;
}

void MapRenderer::RenderRoute(const std::set<const Bus*>& routes, const SphereProjector& proj, svg::Document& doc) const {
    int index_color = 0;
    int count_color = setting_.color_palette.size();
    for (auto route : routes) {
        auto shape = Polyline()
            .SetStrokeColor(setting_.color_palette.at(index_color))
            .SetFillColor(NoneColor)
            .SetStrokeWidth(setting_.line_width)
            .SetStrokeLineCap(StrokeLineCap::ROUND)
            .SetStrokeLineJoin(StrokeLineJoin::ROUND);

        auto& stops = route->stops;
        AddRoutePoints(stops.begin(), stops.end(), shape, proj);
        if (!route->is_roundtrip) {
            AddRoutePoints(std::next(stops.rbegin()), stops.rend(), shape, proj);
        }

        doc.Add(shape);
        ++index_color %= count_color;
    }
}

void MapRenderer::RenderRouteName(const std::set<const Bus*>& routes, const SphereProjector& proj, Document& doc) const {
    int index_color = 0;
    int count_color = setting_.color_palette.size();
    for (auto route : routes) {
        auto stop = route->stops.front();

        auto text = Text()
            .SetPosition(proj(stop->coordinate))
            .SetOffset(setting_.bus_label_offset)
            .SetFontSize(setting_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold")
            .SetData(route->name);

        auto underlay = text;
        CreateUnderlay(underlay, setting_);

        text.SetFillColor(setting_.color_palette.at(index_color));

        while (true) {
            doc.Add(underlay);
            doc.Add(text);

            auto final_stop = stop;
            stop = route->stops.back();

            if (stop->name == final_stop->name) {
                break;
            }
            auto coord = proj(stop->coordinate);
            underlay.SetPosition(coord);
            text.SetPosition(coord);
        }
        ++index_color %= count_color;
    }
}

void MapRenderer::RenderStop(const std::set<const Stop*>& stops, const SphereProjector& proj, svg::Document& doc) const {
    for (auto stop : stops) {
            auto circle = Circle()
                .SetCenter(proj(stop->coordinate))
                .SetRadius(setting_.stop_radius)
                .SetFillColor("white");

            doc.Add(circle);
    }
}

void MapRenderer::RenderStopName(const std::set<const Stop*>& stops, const SphereProjector& proj, svg::Document& doc) const {
    for (auto stop : stops) {
        auto text = Text()
            .SetPosition(proj(stop->coordinate))
            .SetOffset(setting_.stop_label_offset)
            .SetFontSize(setting_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(stop->name);

        auto underlay = text;
        CreateUnderlay(underlay, setting_);

        text.SetFillColor("black");

        doc.Add(underlay);
        doc.Add(text);
    }
}

} // namespace renderer