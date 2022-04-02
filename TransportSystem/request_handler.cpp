#include "request_handler.h"


RequestHandler::RequestHandler(const TransportCatalogue& db, const renderer::MapRender& map_setings, const std::shared_ptr<const transport_router::TransportRouter> router) :
	db_(db),
	render_(map_setings)
{
	router_ = router;
}

svg::Document RequestHandler::RenderMap() const {
	svg::Document result;
	std::deque<geo::Coordinates> coordinstes;
	std::set<std::string_view> set_name_bus;
	std::set<std::string_view> set_name_stop;
	for (const auto& [name_bus, data] : db_.GetBuses()) {
		for (const auto& stop : data->stops) {
			set_name_stop.insert(stop->name);
			coordinstes.push_back(stop->coordinates);
			set_name_bus.insert(name_bus);
		}
	}

	renderer::SphereProjector sphere_projector(coordinstes.begin(), coordinstes.end(), render_.GetSetings().size_.width_, render_.GetSetings().size_.height_, render_.GetSetings().size_.padding_);

	int color_counter = 0;

	for (const auto name_bus : set_name_bus) {
		svg::Polyline vereda = render_.RenderWay(*db_.GetBus(name_bus), color_counter, sphere_projector);
		result.Add(vereda);
		color_counter++;
	}
	color_counter = 0;
	for (const auto name_bus : set_name_bus) {
		if (db_.GetBus(name_bus)->is_loop_trip || db_.GetBus(name_bus)->stops[0] == db_.GetBus(name_bus)->stops[db_.GetBus(name_bus)->stops.size() - 1]) {
			geo::Coordinates begin_stop = db_.GetBus(name_bus)->stops[0]->coordinates;
			std::pair<svg::Text, svg::Text> svg_name_bus_begin = render_.RenderNameBus(name_bus, color_counter, sphere_projector(begin_stop));
			result.Add(svg_name_bus_begin.first);
			result.Add(svg_name_bus_begin.second);
		}
		else {
			geo::Coordinates begin_stop = db_.GetBus(name_bus)->stops[0]->coordinates;
			geo::Coordinates end_stop = db_.GetBus(name_bus)->stops[db_.GetBus(name_bus)->stops.size() - 1]->coordinates;
			std::pair<svg::Text, svg::Text> svg_name_bus_begin = render_.RenderNameBus(name_bus, color_counter, sphere_projector(begin_stop));
			result.Add(svg_name_bus_begin.first);
			result.Add(svg_name_bus_begin.second);
			std::pair<svg::Text, svg::Text> svg_name_bus_end = render_.RenderNameBus(name_bus, color_counter, sphere_projector(end_stop));
			result.Add(svg_name_bus_end.first);
			result.Add(svg_name_bus_end.second);
		}
		color_counter++;
	}

	for (const auto& stop : set_name_stop) {
		svg::Circle circle = render_.RenderStop(sphere_projector(db_.GetStop(stop)->coordinates));
		result.Add(circle);
	}

	for (const auto& stop : set_name_stop) {
		std::pair<svg::Text, svg::Text> svg_name_stop = render_.RenderNameStop(stop, sphere_projector(db_.GetStop(stop)->coordinates));
		result.Add(svg_name_stop.first);
		result.Add(svg_name_stop.second);
	}

	return result;
}

std::optional<transport_router::RouteInfo> RequestHandler::GetRouteStat(const std::string& from, const std::string& to) const {
	return router_->GetRouteInfo(from, to);
}

BusInfo RequestHandler::GetBusStat(std::string_view bus_name) const {
	return db_.GetInfoBus(bus_name);
}