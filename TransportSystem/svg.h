#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <variant>
#include <iomanip>

using namespace std::string_literals;

namespace svg {

    class ObjectContainer;

    class Rgb {
    public:

        Rgb(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0) : red(red), green(green), blue(blue) {
        }

        friend std::ostream& operator<<(std::ostream& out, Rgb rgb);
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    class Rgba {
    public:

        Rgba(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, double opacity = 1.0) : red(red), green(green), blue(blue), opacity(opacity) {
        }

        friend std::ostream& operator<<(std::ostream& out, Rgba rgba);

        uint8_t red;
        uint8_t green;
        uint8_t blue;
        double opacity;
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const Color NoneColor{ "none"s };

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cup);

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_cup);

    std::ostream& operator<<(std::ostream& out, Rgb rgb);

    std::ostream& operator<<(std::ostream& out, Rgba rgba);

    struct Visiter {

        void operator()(std::monostate) const {
            out << "none";
        }
        void operator()(Rgb rgb) const {
            out << rgb;
        }
        void operator()(Rgba rgba) const {
            out << rgba;
        }
        void operator()(std::string color) const {
            out << color;
        }

        std::ostream& out;
    };

    template <typename Owner>
    class PathProps {
    public:

        Owner& SetFillColor(Color color = NoneColor) {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color = NoneColor) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            stroke_width_ = std::move(width);
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap = StrokeLineCap::ROUND) {
            stroke_line_cap_ = std::move(line_cap);
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join = StrokeLineJoin::ROUND) {
            stroke_line_join_ = std::move(line_join);
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (!(std::holds_alternative<std::monostate>(fill_color_))) {
                out << " fill=\""sv;
                std::visit(Visiter{ out }, fill_color_);
                out << "\""sv;
            }

            if (!std::holds_alternative<std::monostate>(stroke_color_)) {
                out << " stroke=\""sv;
                std::visit(Visiter{ out }, stroke_color_);
                out << "\""sv;
            }

            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (stroke_line_cap_) {
                out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
            }
            if (stroke_line_join_) {
                out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            // static_cast áåçîïàñíî ïðåîáðàçóåò *this ê Owner&,
            // åñëè êëàññ Owner — íàñëåäíèê PathProps
            return static_cast<Owner&>(*this);
        }

        Color fill_color_;
        Color stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> stroke_line_cap_;
        std::optional<StrokeLineJoin> stroke_line_join_;
    };

    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Âñïîìîãàòåëüíàÿ ñòðóêòóðà, õðàíÿùàÿ êîíòåêñò äëÿ âûâîäà SVG-äîêóìåíòà ñ îòñòóïàìè.
     * Õðàíèò ññûëêó íà ïîòîê âûâîäà, òåêóùåå çíà÷åíèå è øàã îòñòóïà ïðè âûâîäå ýëåìåíòà
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Àáñòðàêòíûé áàçîâûé êëàññ Object ñëóæèò äëÿ óíèôèöèðîâàííîãî õðàíåíèÿ
     * êîíêðåòíûõ òåãîâ SVG-äîêóìåíòà
     * Ðåàëèçóåò ïàòòåðí "Øàáëîííûé ìåòîä" äëÿ âûâîäà ñîäåðæèìîãî òåãà
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer&) const = 0;

        virtual ~Drawable() = default;
    };

    /*
     * Êëàññ Circle ìîäåëèðóåò ýëåìåíò <circle> äëÿ îòîáðàæåíèÿ êðóãà
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

        Point GetCenter() const {
            return center_;
        }

        double GetRadius() const {
            return radius_;
        }

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    /*
     * Êëàññ Polyline ìîäåëèðóåò ýëåìåíò <polyline> äëÿ îòîáðàæåíèÿ ëîìàíûõ ëèíèé
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Äîáàâëÿåò î÷åðåäíóþ âåðøèíó ê ëîìàíîé ëèíèè
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_ = {};
    };

    /*
     * Êëàññ Text ìîäåëèðóåò ýëåìåíò <text> äëÿ îòîáðàæåíèÿ òåêñòà
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text> {
    public:
        // Çàäà¸ò êîîðäèíàòû îïîðíîé òî÷êè (àòðèáóòû x è y)
        Text& SetPosition(Point pos);

        // Çàäà¸ò ñìåùåíèå îòíîñèòåëüíî îïîðíîé òî÷êè (àòðèáóòû dx, dy)
        Text& SetOffset(Point offset);

        // Çàäà¸ò ðàçìåðû øðèôòà (àòðèáóò font-size)
        Text& SetFontSize(uint32_t size);

        // Çàäà¸ò íàçâàíèå øðèôòà (àòðèáóò font-family)
        Text& SetFontFamily(std::string font_family);

        // Çàäà¸ò òîëùèíó øðèôòà (àòðèáóò font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Çàäà¸ò òåêñòîâîå ñîäåðæèìîå îáúåêòà (îòîáðàæàåòñÿ âíóòðè òåãà text)
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;
        Point pos_;
        Point offset_;
        uint32_t size_ = 1u;
        std::string font_family_ = ""s;
        std::string font_weight_ = ""s;
        std::string data_ = ""s;

        const std::vector<std::pair<char, std::string>> characters_to_replace = {
            {'&', "&amp;"},
            {'"', "&quot;"},
            {'\'', "&apos;"},
            {'<', "&lt;"},
            {'>', "&gt;"}
        };
    };

    class ObjectContainer {
    public:
        /*
         Ìåòîä Add äîáàâëÿåò â svg-äîêóìåíò ëþáîé îáúåêò-íàñëåäíèê svg::Object.
         Ïðèìåð èñïîëüçîâàíèÿ:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Document : public ObjectContainer {
    public:
        // Äîáàâëÿåò â svg-äîêóìåíò îáúåêò-íàñëåäíèê svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        // Âûâîäèò â ostream svg-ïðåäñòàâëåíèå äîêóìåíòà
        void Render(std::ostream& out) const;

        // Ïðî÷èå ìåòîäû è äàííûå, íåîáõîäèìûå äëÿ ðåàëèçàöèè êëàññà Document
    private:
        std::vector<std::unique_ptr<Object>> objects_ = {};
    };

}  // namespace svg