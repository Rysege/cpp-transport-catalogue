#include "svg.h"

namespace svg {

using namespace std::literals;

namespace detail {

void HtmlEncodeString(std::ostream& out, std::string_view str) {
    for (auto c : str) {
        switch (c) {
        case'"':
            out << "&quot;"sv;
            break;
        case'\'':
            out << "&apos;"sv;
            break;
        case'<':
            out << "&lt;"sv;
            break;
        case'>':
            out << "&gt;"sv;
            break;
        case'&':
            out << "&amp;"sv;
            break;
        default:
            out.put(c);
            break;
        }
    }
}

} // namespace detail

std::ostream& operator<<(std::ostream& os, const StrokeLineCap line_cap) {
    switch (line_cap) {
    case svg::StrokeLineCap::BUTT:
        os << "butt"sv;
        break;
    case svg::StrokeLineCap::ROUND:
        os << "round"sv;
        break;
    case svg::StrokeLineCap::SQUARE:
        os << "square"sv;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin line_join) {
    switch (line_join) {
    case svg::StrokeLineJoin::ARCS:
        os << "arcs"sv;
        break;
    case svg::StrokeLineJoin::BEVEL:
        os << "bevel"sv;
        break;
    case svg::StrokeLineJoin::MITER:
        os << "miter"sv;
        break;
    case svg::StrokeLineJoin::MITER_CLIP:
        os << "miter-clip"sv;
        break;
    case svg::StrokeLineJoin::ROUND:
        os << "round"sv;
    }
    return os;
}

void RenderRgb(std::ostream& out, Rgb rgb) {
    out << static_cast<uint16_t>(rgb.red) << ','
        << static_cast<uint16_t>(rgb.green) << ','
        << static_cast<uint16_t>(rgb.blue);
}

void RenderColor(std::ostream& out, std::monostate) {
    out << "none"sv;
}

void RenderColor(std::ostream& out, std::string_view text) {
    out << text;
}

void RenderColor(std::ostream& out, Rgb rgb) {
    out << "rgb("sv;
    RenderRgb(out, rgb);
    out << ')';
}

void RenderColor(std::ostream& out, Rgba rgba) {
    out << "rgba("sv;
    RenderRgb(out, static_cast<Rgb>(rgba));
    out << ',' << rgba.opacity << ')';
}

std::ostream& operator<<(std::ostream& out, const Color color) {
    std::visit([&out](const auto value) { RenderColor(out, value); }, color);
    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (auto p : points_) {
        if (first) {
            first = false;
        }
        else {
            out << ' ';
        }
        out << p.x << ',' << p.y;
    }
    out << '"';
    RenderAttrs(out);
    out << "/>"sv;
}

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_.size = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_.family = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_.weight = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv
        << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv
        << " font-size=\""sv << font_.size << "\""sv;
    if (!font_.family.empty()) {
        out << " font-family=\""sv << font_.family << "\""sv;
    }
    if (!font_.weight.empty()) {
        out << " font-weight=\""sv << font_.weight << "\""sv;
    }
    RenderAttrs(out);
    out.put('>');
    detail::HtmlEncodeString(out, data_);
    out << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    RenderContext ctx(out, 2, 2);
    for (auto& obj : objects_) {
        obj->Render(ctx);
    }
    out << "</svg>"sv;
}

}  // namespace svg

