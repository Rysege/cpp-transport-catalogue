#pragma once
#include "domain.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;

    double line_width = 0.0;
    double stop_radius = 0.0;

    int bus_label_font_size = 0;
    int stop_label_font_size = 0;

    svg::Point bus_label_offset;
    svg::Point stop_label_offset;

    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
         // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
         // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(RenderSettings settings)
        : settings_(settings) {
    }

    svg::Document RenderMap(const std::set<const catalog::Bus*>& routes) const;

private:
    RenderSettings settings_;

    void RenderRoute(const std::set<const catalog::Bus*>& routes, const SphereProjector& proj, svg::Document& doc) const;
    void RenderRouteName(const std::set<const catalog::Bus*>& routes, const SphereProjector& proj, svg::Document& doc) const;
    void RenderStop(const std::set<const catalog::Stop*>& stops, const SphereProjector& proj, svg::Document& doc) const;
    void RenderStopName(const std::set<const catalog::Stop*>& stops, const SphereProjector& proj, svg::Document& doc) const;
};

template <typename Iterator>
void AddRoutePoints(Iterator first, Iterator last, svg::Polyline& shape, const SphereProjector& proj) {
    while (first != last) {
        shape.AddPoint(proj((*first++)->coordinate));
    }
}

template <typename ReturnType, typename Lambda>
auto ExtractData(const std::set<const catalog::Bus*>& routes, Lambda lambda) {
    ReturnType result{};
    for (auto route : routes) {
        std::transform(
            route->stops.begin(), route->stops.end(), std::inserter(result, end(result)), lambda);
    }
    return result;
}

} // namespace renderer