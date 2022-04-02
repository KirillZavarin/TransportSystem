#include "map_renderer.h"


namespace renderer {

	bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	MapSettings::MapSettings(MapSize map_size, ElementSize element_size, BusLabel bus_label,
		StopLabel stop_label, const svg::Color& underlayer_color,
		double underlayer_width, const std::vector<svg::Color>& color_palette) :
		size_(map_size),
		element_size_(element_size),
		bus_label_(bus_label),
		stop_label_(stop_label),
		underlayer_color_(underlayer_color),
		underlayer_width_(underlayer_width),
		color_palette_(color_palette) {
	}

	svg::Polyline MapRender::RenderWay(const transportcatalogue::Bus& bus, int color_counter, const renderer::SphereProjector& sphere_projector) const {
		svg::Polyline vereda;
		vereda.SetStrokeWidth(settings_.element_size_.line_width_).SetStrokeColor(settings_.color_palette_[color_counter % settings_.color_palette_.size()]).SetFillColor().SetStrokeLineCap().SetStrokeLineJoin();
		if (bus.is_loop_trip) {
			for (const auto& stop : bus.stops) {
				vereda.AddPoint(sphere_projector(stop->coordinates));
			}
		}
		else {
			auto stops = bus.stops;
			auto rstops = bus.stops;
			rstops.pop_back();
			std::reverse(rstops.begin(), rstops.end());
			for (const auto& stop : rstops) {
				stops.push_back(stop);
			}
			for (const auto& stop : stops) {
				vereda.AddPoint(sphere_projector(stop->coordinates));
			}
		}
		return vereda;
	}

	std::pair<svg::Text, svg::Text> MapRender::RenderNameBus(std::string_view name_bus, int color_counter, svg::Point stop) const {
		svg::Text background;
		svg::Text title;
		background.SetData(std::string(name_bus)).
			SetFillColor(settings_.underlayer_color_).
			SetStrokeColor(settings_.underlayer_color_).
			SetStrokeWidth(settings_.underlayer_width_).
			SetStrokeLineCap().SetStrokeLineJoin().
			SetPosition(stop).
			SetOffset(svg::Point(settings_.bus_label_.bus_label_offset_[0], settings_.bus_label_.bus_label_offset_[1])).
			SetFontSize(settings_.bus_label_.bus_label_font_size_).
			SetFontFamily("Verdana"s).SetFontWeight("bold"s);
		title.SetData(std::string(name_bus)).
			SetFillColor(settings_.color_palette_[color_counter % settings_.color_palette_.size()]).
			SetPosition(stop).
			SetOffset(svg::Point(settings_.bus_label_.bus_label_offset_[0], settings_.bus_label_.bus_label_offset_[1])).
			SetFontSize(settings_.bus_label_.bus_label_font_size_).
			SetFontFamily("Verdana"s).SetFontWeight("bold"s);
		return { background , title };
	}

	svg::Circle MapRender::RenderStop(const svg::Point& stop) const {
		svg::Circle result;
		result.SetCenter(stop).SetRadius(settings_.element_size_.stop_radius_).SetFillColor("white");
		return result;
	}

	std::pair<svg::Text, svg::Text> MapRender::RenderNameStop(const std::string_view name_stop, const svg::Point& stop) const {
		svg::Text background;
		svg::Text title;
		background.SetData(std::string(name_stop)).
			SetFillColor(settings_.underlayer_color_).
			SetStrokeColor(settings_.underlayer_color_).
			SetStrokeWidth(settings_.underlayer_width_).
			SetStrokeLineCap().SetStrokeLineJoin().
			SetPosition(stop).
			SetOffset(svg::Point(settings_.stop_label_.stop_label_offset_[0], settings_.stop_label_.stop_label_offset_[1])).
			SetFontSize(settings_.stop_label_.stop_label_font_size_).
			SetFontFamily("Verdana"s);
		title.SetData(std::string(name_stop)).
			SetFillColor("black"s).
			SetPosition(stop).
			SetOffset(svg::Point(settings_.stop_label_.stop_label_offset_[0], settings_.stop_label_.stop_label_offset_[1])).
			SetFontSize(settings_.stop_label_.stop_label_font_size_).
			SetFontFamily("Verdana"s);
		return { background , title };
	}

	svg_serialize::Color MapRender::SetProtoColor(const svg::Color& color) {
		svg_serialize::Color proto__color;
		switch (color.index())
		{
		case 0:
			proto__color.set_monostate(true);
			break;
		case 1:
			proto__color.set_str(std::get<std::string>(color));
			break;
		case 2:
			*proto__color.mutable_rgb() = SetProtoRgb(std::get<svg::Rgb>(color));
			break;
		case 3:
			*proto__color.mutable_rgba() = SetProtoRgba(std::get<svg::Rgba>(color));
			break;
		default:
			break;
		}
		return proto__color;
	}

	svg_serialize::Rgb MapRender::SetProtoRgb(const svg::Rgb& color) {
		svg_serialize::Rgb proto_rgb;
		proto_rgb.set_blue(color.blue);
		proto_rgb.set_red(color.red);
		proto_rgb.set_green(color.green);
		return proto_rgb;
	}

	svg_serialize::Rgba MapRender::SetProtoRgba(const svg::Rgba& color) {
		svg_serialize::Rgba proto_rgba;
		proto_rgba.set_blue(color.blue);
		proto_rgba.set_red(color.red);
		proto_rgba.set_green(color.green);
		proto_rgba.set_opacity(color.opacity);
		return proto_rgba;
	}

	svg::Color MapRender::SetSvgColor(const svg_serialize::Color& proto_color) {
		svg::Color color;
		switch (proto_color.variant_case())
		{
		case 1:
			break;
		case 2:
			color = proto_color.str();
			break;
		case 3:
			color = SetSvgRgb(proto_color.rgb());
			break;
		case 4:
			color = SetSvgRgba(proto_color.rgba());
			break;
		default:
			break;
		}
		return color;
	}
	svg::Rgb MapRender::SetSvgRgb(const svg_serialize::Rgb& proto_color) {
		return svg::Rgb{ static_cast<uint8_t>(proto_color.red()), static_cast<uint8_t>(proto_color.green()), static_cast<uint8_t>(proto_color.blue()) };
	}
	svg::Rgba MapRender::SetSvgRgba(const svg_serialize::Rgba& proto_color) {
		return svg::Rgba{ static_cast<uint8_t>(proto_color.red()), static_cast<uint8_t>(proto_color.green()), static_cast<uint8_t>(proto_color.blue()), proto_color.opacity() };
	}

	map_renderer_serialize::MapSettings MapRender::SaveToProto() const {
		map_renderer_serialize::MapSettings result;
		//add map size;
		{
			map_renderer_serialize::MapSize proto_size;
			proto_size.set_width_(settings_.size_.width_);
			proto_size.set_height_(settings_.size_.height_);
			proto_size.set_padding_(settings_.size_.padding_);
			*result.mutable_size_() = proto_size;
		}
		//add bus label;
		{
			map_renderer_serialize::BusLabel proto_bus_label;
			proto_bus_label.set_bus_label_font_size_(settings_.bus_label_.bus_label_font_size_);
			for (double i : settings_.bus_label_.bus_label_offset_) {
				proto_bus_label.add_bus_label_offset_(i);
			}
			*result.mutable_bus_label_() = proto_bus_label;
		}
		//add stop label;
		{
			map_renderer_serialize::StopLabel proto_stop_label;
			proto_stop_label.set_stop_label_font_size_(settings_.stop_label_.stop_label_font_size_);
			for (double i : settings_.stop_label_.stop_label_offset_) {
				proto_stop_label.add_stop_label_offset_(i);
			}
			*result.mutable_stop_label_() = proto_stop_label;
		}
		//add element size
		{
			map_renderer_serialize::ElementSize proto_el_size;
			proto_el_size.set_line_width_(settings_.element_size_.line_width_);
			proto_el_size.set_stop_radius_(settings_.element_size_.stop_radius_);
			*result.mutable_element_size_() = proto_el_size;
		}

		*result.mutable_underlayer_color_() = SetProtoColor(settings_.underlayer_color_);
		result.set_underlayer_width_(settings_.underlayer_width_);
		for (const svg::Color& color : settings_.color_palette_) {
			*result.add_color_palette_() = SetProtoColor(color);
		}
		return result;
	}

	MapRender DeserializeMapRender(const map_renderer_serialize::MapSettings& proto_map_setting) {
		MapSettings::MapSize map_size(proto_map_setting.size_().width_(), proto_map_setting.size_().height_(), proto_map_setting.size_().padding_());
		double bus_label_offset[2] = { proto_map_setting.bus_label_().bus_label_offset_()[0], proto_map_setting.bus_label_().bus_label_offset_()[1] };
		MapSettings::BusLabel bus_label(proto_map_setting.bus_label_().bus_label_font_size_(), bus_label_offset);
		double stop_label_offset[2] = { proto_map_setting.stop_label_().stop_label_offset_()[0], proto_map_setting.stop_label_().stop_label_offset_()[1] };
		MapSettings::StopLabel stop_label(proto_map_setting.stop_label_().stop_label_font_size_(), stop_label_offset);
		MapSettings::ElementSize element_size(proto_map_setting.element_size_().line_width_(), proto_map_setting.element_size_().stop_radius_());
		svg::Color underlayer_color = MapRender::SetSvgColor(proto_map_setting.underlayer_color_());
		std::vector<svg::Color> color_palette;
		for (const svg_serialize::Color& proto_color : proto_map_setting.color_palette_()) {
			color_palette.push_back(MapRender::SetSvgColor(proto_color));
		}
		return MapRender{ MapSettings{ map_size, element_size, bus_label, stop_label, underlayer_color, proto_map_setting.underlayer_width_(), color_palette } };
	}
}