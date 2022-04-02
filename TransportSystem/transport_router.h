#pragma once
#include <unordered_map>
#include <vector>
#include <string_view>
#include <exception>
#include <optional>
#include <variant>

#include <transport_router.pb.h>

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

using namespace std::string_literals;

using namespace std::string_view_literals;

namespace transport_router {
	class TransportRouter;

	class RouteInfo {

	private:
		struct ComponentTrip {
			ComponentTrip() = default;

			ComponentTrip(std::string_view name, double weight, std::optional<unsigned int> span_count) :
				name_(name),
				weight_(weight),
				span_count_(span_count) {
			}

			ComponentTrip(const ComponentTrip& other) = default;

			std::string_view name_ = ""sv;
			double weight_ = 0;
			std::optional<unsigned int> span_count_ = std::nullopt;
		};

		std::vector<ComponentTrip> items_;
		double total_time = 0.;
	public:

		friend class TransportRouter;

		friend transport_router::TransportRouter* DeserializeTransportRouter(const router_serialize::TransportRouter& proto_router, const transportcatalogue::TransportCatalogue& db);

		void AddRideItem(std::string_view bus_name, unsigned int span_count, double ride_time) {
			items_.push_back(ComponentTrip(bus_name, ride_time, span_count));
		}

		void AddWaitItem(std::string_view stop_name, double time_wait) {
			items_.push_back(ComponentTrip(stop_name, time_wait, std::nullopt));
		}

		double GetTotalTime() {
			return total_time;
		}

		void AdditionTotalTime(double time) {
			total_time += time;
		}

		std::vector<ComponentTrip> GetItems() {
			return items_;
		}
	};

	struct RoutingSettings {

		RoutingSettings(unsigned short int bus_wait_time, unsigned short int bus_velocity) :
			bus_wait_time_(bus_wait_time),
			bus_velocity_(bus_velocity) {
		};

		unsigned short int bus_wait_time_;
		unsigned short int bus_velocity_;
	};

	struct VertexId {
		graph::VertexId in;
		graph::VertexId out;
	};

	class TransportRouter {
		friend transport_router::TransportRouter* DeserializeTransportRouter(const router_serialize::TransportRouter& proto_router, const transportcatalogue::TransportCatalogue& db);
	public:
		TransportRouter(const transportcatalogue::TransportCatalogue& db, graph::DirectedWeightedGraph<double>&& graph_of_stop, graph::Router<double>::RoutesInternalData&& routes_internal_data)
			: settings_(RoutingSettings{ 0, 0 })
			, db_(db)
			, graph_of_stops(std::move(graph_of_stop))
			, router_ptr_(new graph::Router(&graph_of_stops, std::move(routes_internal_data))) {

		}

		TransportRouter(RoutingSettings rs, const transportcatalogue::TransportCatalogue& db);

		~TransportRouter() {
			delete router_ptr_;
		}

		std::optional<RouteInfo> GetRouteInfo(std::string_view from, std::string_view to) const;

		router_serialize::TransportRouter SaveToProto() const;
	private:
		router_serialize::RoutingSettings SaveRoutingSettingsToProto() const;

		router_serialize::DirectedWeightedGraph SaveGraphToProto() const;

		router_serialize::Router SaveRouterToProto() const;

		void CreateGraph();

		void CreateVertex(const transportcatalogue::Stop& stop);

		void CreateEdge(const transportcatalogue::Bus& bus);

		double CalculateTimeBetweenStations(std::string_view first_stop, std::string_view second_stop) const;

		RoutingSettings settings_;

		const transportcatalogue::TransportCatalogue& db_;

		graph::VertexId total_vertex = 0;

		graph::DirectedWeightedGraph<double> graph_of_stops = {};

		graph::Router<double>* router_ptr_ = nullptr;

		std::unordered_map<std::string_view, VertexId> stop_vertexs_ = {};

		std::unordered_map<graph::EdgeId, RouteInfo::ComponentTrip> info_about_edge = {};
	};

	transport_router::TransportRouter* DeserializeTransportRouter(const router_serialize::TransportRouter& proto_router, const transportcatalogue::TransportCatalogue& db);
}
