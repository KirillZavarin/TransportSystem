#pragma once

#include <algorithm>
#include <transport_catalogue.pb.h>

#include "svg.h"
#include "geo.h"
#include "domain.h"

namespace renderer {
    inline const double EPSILON = 1e-6;
    bool IsZero(double value);

    class MapRender;

    class SphereProjector {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
            double max_height, double padding)
            : padding_(padding) {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it]
                = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lng < rhs.lng;
                    });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            const auto [bottom_it, top_it]
                = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                return lhs.lat < rhs.lat;
                    });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                zoom_coeff_ = *height_zoom;
            }
        }

        svg::Point operator()(geo::Coordinates coords) const {
            return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };

    struct MapSettings {
        friend MapRender DeserializeMapRender(const map_renderer_serialize::MapSettings& proto_map_setting);
    private:
        struct MapSize {
            MapSize() = default;
            MapSize(double width, double height, double padding) : width_(width), height_(height), padding_(padding) {}

            double width_ = 0.;
            double height_ = 0.;
            double padding_ = 0.;
        };

        struct BusLabel {
            BusLabel() = default;
            BusLabel(int bus_label_font_size, double bus_label_offset[2]) : bus_label_font_size_(bus_label_font_size) {
                bus_label_offset_[0] = bus_label_offset[0];
                bus_label_offset_[1] = bus_label_offset[1];
            }

            int bus_label_font_size_ = 0;
            double bus_label_offset_[2] = { 0., 0. };
        };

        struct StopLabel {
            StopLabel() = default;
            StopLabel(int stop_label_font_size, double stop_label_offset[2]) : stop_label_font_size_(stop_label_font_size) {
                stop_label_offset_[0] = stop_label_offset[0];
                stop_label_offset_[1] = stop_label_offset[1];
            }

            int stop_label_font_size_ = 0;
            double stop_label_offset_[2] = { 0., 0. };
        };

        struct ElementSize {
            ElementSize() = default;
            ElementSize(double line_width, double stop_radius) :
                line_width_(line_width),
                stop_radius_(stop_radius) {
            }
            double line_width_ = 0.;
            double stop_radius_ = 0.;
        };

    public:
        MapSettings() = default;

        MapSettings(MapSize map_size, ElementSize element_size, BusLabel bus_label, StopLabel stop_label, const svg::Color& underlayer_color,
            double underlayer_width, const std::vector<svg::Color>& color_palette);

        MapSize size_ = {};
        ElementSize element_size_ = {};
        BusLabel bus_label_ = {};
        StopLabel stop_label_ = {};
        svg::Color underlayer_color_ = {};
        double underlayer_width_ = 0.;
        std::vector<svg::Color> color_palette_ = {};
    };

    class MapRender {
        friend MapRender DeserializeMapRender(const map_renderer_serialize::MapSettings& proto_map_setting);
    public:
        MapRender() = default;

        MapRender(const MapSettings& setings) :
            settings_(setings) {
        }

        const MapSettings& GetSetings() const {
            return settings_;
        }

        void SetSettings(const MapSettings& setings) {
            settings_ = setings;
        }

        svg::Polyline RenderWay(const transportcatalogue::Bus& bus, int color_counter, const renderer::SphereProjector& sphere_projector) const;

        std::pair<svg::Text, svg::Text> RenderNameBus(std::string_view name_bus, int color_counter, svg::Point stop) const;

        svg::Circle RenderStop(const svg::Point& stop) const;

        std::pair<svg::Text, svg::Text> RenderNameStop(const std::string_view name_stop, const svg::Point& stop) const;

        map_renderer_serialize::MapSettings SaveToProto() const;
    private:
        static svg_serialize::Color SetProtoColor(const svg::Color& color);
        static svg_serialize::Rgb SetProtoRgb(const svg::Rgb& color);
        static svg_serialize::Rgba SetProtoRgba(const svg::Rgba& color);

        static svg::Color SetSvgColor(const svg_serialize::Color& proto_color);
        static svg::Rgb SetSvgRgb(const svg_serialize::Rgb& proto_color);
        static svg::Rgba SetSvgRgba(const svg_serialize::Rgba& proto_color);
        MapSettings settings_ = {};
    };
    MapRender DeserializeMapRender(const map_renderer_serialize::MapSettings& proto_map_setting);
}