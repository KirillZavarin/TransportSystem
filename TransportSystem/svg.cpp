#include "svg.h"

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Äåëåãèðóåì âûâîä òåãà ñâîèì ïîäêëàññàì
        RenderObject(context);

        context.out << std::endl;
    }

    //------------------- output operators ----------

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cup) {
        if (line_cup == StrokeLineCap::BUTT) {
            out << "butt"sv;
        }
        if (line_cup == StrokeLineCap::ROUND) {
            out << "round"sv;
        }
        if (line_cup == StrokeLineCap::SQUARE) {
            out << "square"sv;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_cup) {
        if (line_cup == StrokeLineJoin::ARCS) {
            out << "arcs"sv;
        }
        if (line_cup == StrokeLineJoin::BEVEL) {
            out << "bevel"sv;
        }
        if (line_cup == StrokeLineJoin::MITER) {
            out << "miter"sv;
        }
        if (line_cup == StrokeLineJoin::MITER_CLIP) {
            out << "miter-clip"sv;
        }
        if (line_cup == StrokeLineJoin::ROUND) {
            out << "round"sv;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, Rgb rgb) {
        out << "rgb(" << unsigned(rgb.red) << ',' << unsigned(rgb.green) << ',' << unsigned(rgb.blue) << ')';
        return out;
    }

    std::ostream& operator<<(std::ostream& out, Rgba rgba) {
        out << "rgba(" << unsigned(rgba.red) << ',' << unsigned(rgba.green) << ',' << unsigned(rgba.blue) << ',' << rgba.opacity << ')';
        return out;
    }

    // ---------------- Circle ------------------

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

    //---------- Polyline -----------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(std::move(point));
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        for (auto it = points_.begin(); it < points_.end(); it++) {
            if (it == points_.begin()) {
                out << it->x << ","sv << it->y;
            }
            else {
                out << " "sv << it->x << ","sv << it->y;
            }
        }

        out << "\"";

        RenderAttrs(out);

        out << "/>"sv;
    }

    //------------ Text ----------------

    Text& Text::SetPosition(Point pos) {
        pos_ = std::move(pos);
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = std::move(offset);
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        size_ = std::move(size);
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {

        for (const auto& [symvol, to] : characters_to_replace) {
            size_t pos = 0;
            while (true) {
                pos = data.find(symvol, pos + 1);

                if (pos == data.npos) {
                    break;
                }

                std::string after_pos = to + data.substr(pos + 1);

                data.replace(pos, data.npos, after_pos);
            }
        }

        data_ = std::move(data);

        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;

        out << "<text"sv;

        RenderAttrs(out);

        out << " x=\""sv << this->pos_.x << "\" y=\""sv << this->pos_.y << "\""sv
            << " dx=\""sv << this->offset_.x << "\" dy=\""sv << this->offset_.y << "\""sv
            << " font-size=\"" << this->size_ << "\""sv;
        if (!font_family_.empty()) {
            out << " font-family=\""sv << this->font_family_ << "\""sv;
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << this->font_weight_ << "\""sv;
        }

        out << ">"sv << this->data_ << "</text>"sv;
    }

    //------------- Document -----------------

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        RenderContext context(out, 2, 2);

        context.out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        context.out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        for (const auto& obj : objects_) {
            obj->Render(context);
        }

        context.out << "</svg>"sv;
    }

}  // namespace svg