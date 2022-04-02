#include "transport_router.h"

namespace transport_router {

	TransportRouter::TransportRouter(RoutingSettings rs, const transportcatalogue::TransportCatalogue& db) :
		settings_(rs),
		db_(db) {
		graph_of_stops = graph::DirectedWeightedGraph<double>(db.GetAllStops().size() * 2);
		CreateGraph();
		router_ptr_ = new graph::Router<double>(graph_of_stops);
	}

	void TransportRouter::CreateGraph() {
		for (const transportcatalogue::Stop& stop : db_.GetAllStops()) {
			CreateVertex(stop);
		}
		for (const transportcatalogue::Bus& bus : db_.GetAllBuses()) {
			CreateEdge(bus);
		}
	}

	void TransportRouter::CreateVertex(const transportcatalogue::Stop& stop) {

		stop_vertexs_[stop.name].in = total_vertex++;
		stop_vertexs_[stop.name].out = total_vertex++;
		info_about_edge.emplace(graph_of_stops.AddEdge({ stop_vertexs_.at(stop.name).in,
														 stop_vertexs_.at(stop.name).out,
														 static_cast<double>(settings_.bus_wait_time_) }),
			RouteInfo::ComponentTrip(stop.name, settings_.bus_wait_time_,
				std::nullopt));
	}

	void TransportRouter::CreateEdge(const transportcatalogue::Bus& bus) {
		for (auto from_it = bus.stops.begin(); from_it != bus.stops.end(); from_it++) {
			double time_to_road = 0;
			int stops_count = 0;
			graph::VertexId first_id = stop_vertexs_.at((*from_it)->name).out;
			for (auto to_it = std::next(from_it); to_it != bus.stops.end(); to_it++) {
				graph::VertexId second_id = stop_vertexs_.at((*to_it)->name).in;

				time_to_road += CalculateTimeBetweenStations((*std::prev(to_it))->name, (*to_it)->name);
				stops_count++;


				info_about_edge.emplace(graph_of_stops.AddEdge({ first_id, second_id, time_to_road }), RouteInfo::ComponentTrip(bus.name, time_to_road, stops_count));
			}
		}
		if (!bus.is_loop_trip) {
			for (auto from_it = bus.stops.rbegin(); from_it != bus.stops.rend(); from_it++) {
				double time_to_road = 0;
				int stops_count = 0;
				graph::VertexId first_id = stop_vertexs_.at((*from_it)->name).out;
				for (auto to_it = std::next(from_it); to_it != bus.stops.rend(); to_it++) {
					graph::VertexId second_id = stop_vertexs_.at((*to_it)->name).in;

					time_to_road += CalculateTimeBetweenStations((*std::prev(to_it))->name, (*to_it)->name);
					stops_count++;


					info_about_edge.emplace(graph_of_stops.AddEdge({ first_id, second_id, time_to_road }), RouteInfo::ComponentTrip(bus.name, time_to_road, stops_count));
				}
			}
		}
	}

	std::optional<RouteInfo> TransportRouter::GetRouteInfo(std::string_view from, std::string_view to) const {
		if (stop_vertexs_.find(from) == stop_vertexs_.end()) {
			throw std::logic_error("there is no starting stop"s);
		}
		RouteInfo result;

		std::optional<graph::Router<double>::RouteInfo> route_info = router_ptr_->BuildRoute(stop_vertexs_.at(from).in, stop_vertexs_.at(to).in);

		if (!route_info) {
			return {};
		}

		for (graph::EdgeId id : route_info.value().edges) {
			RouteInfo::ComponentTrip info = info_about_edge.at(id);
			info.span_count_.has_value() ? result.AddRideItem(info.name_, info.span_count_.value(), info.weight_) : result.AddWaitItem(info.name_, info.weight_);
			result.AdditionTotalTime(info.weight_);
		}

		return result;
	}

	double TransportRouter::CalculateTimeBetweenStations(std::string_view first_stop, std::string_view second_stop) const {
		return db_.GetLenghtBetweenStops(first_stop, second_stop) / (1000.0 * settings_.bus_velocity_) * 60.0;
	}

	router_serialize::RoutingSettings TransportRouter::SaveRoutingSettingsToProto() const {
		router_serialize::RoutingSettings proto_settings;
		proto_settings.set_bus_wait_time_(settings_.bus_wait_time_);
		proto_settings.set_bus_velocity_(settings_.bus_velocity_);
		return proto_settings;
	}

	router_serialize::DirectedWeightedGraph TransportRouter::SaveGraphToProto() const {
		router_serialize::DirectedWeightedGraph proto_graph_of_stops;
		for (size_t i = 0; i < graph_of_stops.GetEdgeCount(); i++) {
			router_serialize::Edge proto_edge;
			const auto& edge = graph_of_stops.GetEdge(i);
			proto_edge.set_from(edge.from);
			proto_edge.set_to(edge.from);
			proto_edge.set_weight(edge.weight);
			*proto_graph_of_stops.add_edges_() = proto_edge;
		}
		for (const auto& incidence_lists : graph_of_stops.GetIncidenceLists()) {
			router_serialize::IncidenceList proto_incidence_lists_;
			for (const auto& EdgeId : incidence_lists) {
				proto_incidence_lists_.add_edgeids(EdgeId);
			}
			*proto_graph_of_stops.add_incidence_lists_() = proto_incidence_lists_;
		}
		return proto_graph_of_stops;
	}

	router_serialize::Router TransportRouter::SaveRouterToProto() const {
		router_serialize::Router proto_router;
		for (const auto& internal_list : router_ptr_->GetRoutesInternalData()) {
			router_serialize::RoutesInternalData proto_internal_list;
			for (const auto& data : internal_list) {
				if (data.has_value()) {
					router_serialize::RouteInternalData proto_data;
					proto_data.set_weight(data.value().weight);
					if (data.value().prev_edge.has_value()) {
						proto_data.mutable_prev_edge()->set_data(data.value().prev_edge.value());
					}
					proto_data.set_is_initialisation(true);
					*proto_internal_list.add_data() = proto_data;
				}
				else {
					router_serialize::RouteInternalData proto_data;
					proto_data.set_is_initialisation(false);
					*proto_internal_list.add_data() = proto_data;
				}
			}
			*proto_router.add_routes_internal_data_() = proto_internal_list;
		}
		return proto_router;
	}

	router_serialize::TransportRouter TransportRouter::SaveToProto() const {
		router_serialize::TransportRouter result;
		std::map<std::string_view, uint32_t> name_stops;

		for (const auto& stop : this->db_.GetAllStops()) {
			result.add_stops(stop.name);
			name_stops[stop.name] = result.stops_size() - 1;
		}

		result.set_total_vertex_(total_vertex);

		*result.mutable_settings_() = SaveRoutingSettingsToProto();

		*result.mutable_graph_of_stops_() = SaveGraphToProto();

		*result.mutable_router() = SaveRouterToProto();

		for (const auto& [name, id] : stop_vertexs_) {
			router_serialize::VertexId proto_id;
			proto_id.set_in(id.in);
			proto_id.set_out(id.out);
			(*result.mutable_stop_vertexs_())[name_stops.at(name)] = proto_id;
		}

		for (const auto& [id, component] : info_about_edge) {
			router_serialize::ComponentTrip proto_component_trip;
			proto_component_trip.set_name(std::string(component.name_));
			proto_component_trip.set_weight_(component.weight_);
			if (component.span_count_.has_value()) {
				proto_component_trip.mutable_span_count_()->set_data(component.span_count_.value());
			}
			(*result.mutable_info_about_edge())[id] = proto_component_trip;
		}

		return result;
	}

	transport_router::TransportRouter* DeserializeTransportRouter(const router_serialize::TransportRouter& proto_router, const transportcatalogue::TransportCatalogue& db) {

		graph::Router<double>::RoutesInternalData routes_internal_data;
		std::vector<graph::Edge<double>> edges;
		std::vector<std::vector<graph::EdgeId>> incidence_lists;
		{
			for (const auto& proto_edge : proto_router.graph_of_stops_().edges_()) {
				graph::Edge<double> edge;
				edge.from = proto_edge.from();
				edge.to = proto_edge.to();
				edge.weight = proto_edge.weight();
				edges.push_back(std::move(edge));
			}

			for (const auto& proto_incidence_lists : proto_router.graph_of_stops_().incidence_lists_()) {
				std::vector<graph::EdgeId> EdgeIds;
				for (uint32_t id : proto_incidence_lists.edgeids()) {
					EdgeIds.push_back(id);
				}
				incidence_lists.push_back(std::move(EdgeIds));
			}
			std::vector<std::optional<graph::Router<double>::RouteInternalData>> internal_list;
			for (const auto& proto_internal_list : proto_router.router().routes_internal_data_()) {
				for (const auto& proto_internal : proto_internal_list.data()) {

					if (proto_internal.is_initialisation()) {
						graph::Router<double>::RouteInternalData data;
						data.weight = proto_internal.weight();
						if (proto_internal.has_prev_edge()) {
							data.prev_edge = proto_internal.prev_edge().data();
						}
						else {
							data.prev_edge = std::nullopt;
						}
						internal_list.push_back(std::move(data));
					}
					else {
						internal_list.push_back(std::nullopt);
					}
				}
				routes_internal_data.push_back(std::move(internal_list));
			}
		}
		graph::DirectedWeightedGraph<double> graph_of_stop(std::move(edges), std::move(incidence_lists));
		transport_router::TransportRouter* result = new TransportRouter(db, std::move(graph_of_stop), std::move(routes_internal_data));

		{
			RoutingSettings settings(proto_router.settings_().bus_wait_time_(), proto_router.settings_().bus_velocity_());
			result->settings_ = settings;
		}
		result->total_vertex = proto_router.total_vertex_();


		for (const auto& [name, id] : proto_router.stop_vertexs_()) {

			result->stop_vertexs_[db.GetStop(proto_router.stops()[name])->name].in = id.in();
			result->stop_vertexs_[db.GetStop(proto_router.stops()[name])->name].out = id.out();
		}
		for (const auto& [id, component] : proto_router.info_about_edge()) {

			if (component.has_span_count_()) {

				RouteInfo::ComponentTrip info(db.GetBus(component.name())->name, component.weight_(), component.span_count_().data());
				result->info_about_edge[id] = info;
			}
			else {
				RouteInfo::ComponentTrip info(db.GetStop(component.name())->name, component.weight_(), std::nullopt);
				result->info_about_edge[id] = info;
			}
		}
		return result;
	}
}