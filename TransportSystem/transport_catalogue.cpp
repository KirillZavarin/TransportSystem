#include "transport_catalogue.h"

namespace transportcatalogue {

	void TransportCatalogue::AddBus(const std::string_view name, std::vector<std::string_view>&& stops, bool is_loop) {
		std::string bus_name(name);
		std::deque<Stop*> deq_stops;
		Bus bus = { bus_name, {} , is_loop };
		all_buses_.push_back(bus);
		buses_[all_buses_.back().name] = &all_buses_.back();

		for (auto& stop : stops) {
			if (stops_.count(stop) != 0) {
				deq_stops.push_back(stops_.at(stop));
			}
			else {
				AddStop(stop, {});
				deq_stops.push_back(stops_.at(stop));
			}
			if (std::find(buses_in_stop_[stops_.at(stop)->name].begin(), buses_in_stop_[stops_.at(stop)->name].end(), buses_.at(name)->name) == buses_in_stop_[stops_.at(stop)->name].end()) {
				buses_in_stop_[stops_.at(stop)->name].push_back(buses_.at(name)->name);
			}
		}

		buses_.at(name)->stops = deq_stops;
	}

	void TransportCatalogue::AddStop(const std::string_view name, const geo::Coordinates& location) {
		std::string stop_name(name);
		Stop stop = { stop_name, location };
		if (stops_.count(name) != 0) {
			if (stops_.at(name)->coordinates.lat == 0. && stops_.at(name)->coordinates.lng == 0.) {
				*stops_.at(name) = stop;
			}
		}
		else {
			all_stops_.push_back(stop);
			stops_[all_stops_.back().name] = &all_stops_.back();
		}
	}

	void TransportCatalogue::AddLenghtBetweenStops(const std::pair<std::string_view, std::string_view>& pair_stops, uint32_t lenght) {
		Stop* p_stop_1 = stops_.at(pair_stops.first);
		if (stops_.find(pair_stops.second) == stops_.end()) {
			AddStop(pair_stops.second);
		}
		Stop* p_stop_2 = stops_.at(pair_stops.second);
		length_between_stops_.insert({ { p_stop_1 , p_stop_2 }, lenght });
	}

	[[nodiscard]] const BusInfo TransportCatalogue::GetInfoBus(std::string_view bus_name) const {
		if (!BusAvailability(bus_name)) {
			return { false, 0., 0, 0, 0 };
		}
		else {
			int quantity_stop;
			int unique_stops;

			std::unordered_set<Stop*> stops;

			for (const auto& stop : buses_.at(bus_name)->stops) {
				stops.insert(stop);
			}

			unique_stops = stops.size();

			if (buses_.at(bus_name)->is_loop_trip) {
				quantity_stop = static_cast<int>(buses_.at(bus_name)->stops.size());
			}
			else {
				quantity_stop = static_cast<int>(buses_.at(bus_name)->stops.size()) * 2 - 1;
			}

			double route_length = SummationLenght(*buses_.at(bus_name));
			double curvature = static_cast<double>(route_length) / SummationLineLenght(*buses_.at(bus_name));

			return { true, curvature, route_length, quantity_stop, unique_stops };
		}
	}

	[[nodiscard]] const StopInfo TransportCatalogue::GetInfoStop(std::string_view stop_name) const {
		if (!StopAvailability(stop_name)) {
			return { false, {} };
		}
		else if (buses_in_stop_.count(stop_name) == 0) {
			return { true, {} };
		}
		else {
			StopInfo result = { true, buses_in_stop_.at(stop_name) };
			std::sort(result.buses.begin(), result.buses.end());
			return result;
		}
	}

	double TransportCatalogue::SummationLenght(const Bus& bus) const {
		if (bus.is_loop_trip) {
			return std::transform_reduce(std::next(bus.stops.begin()), bus.stops.end(), bus.stops.begin(), 0, std::plus<>{}, [&](Stop* lhs, Stop* rhs) {
				std::pair<Stop*, Stop*> pair_stops({ rhs , lhs });
				if (length_between_stops_.count(pair_stops) != 0) {
					return length_between_stops_.at(pair_stops);
				}
				return length_between_stops_.at({ lhs , rhs });
				}
			);
		}
		else {
			auto stops = bus.stops;
			auto rstops = bus.stops;
			rstops.pop_back();
			std::reverse(rstops.begin(), rstops.end());
			for (const auto& stop : rstops) {
				stops.push_back(stop);
			}
			return std::transform_reduce(std::next(stops.begin()), stops.end(), stops.begin(), 0, std::plus<>{}, [&](Stop* lhs, Stop* rhs) {
				std::pair<Stop*, Stop*> pair_stops({ rhs , lhs });
				std::pair<Stop*, Stop*> revers_pair_stops({ lhs , rhs });
				if (length_between_stops_.count(pair_stops) != 0) {
					return length_between_stops_.at(pair_stops);
				}
				else if (length_between_stops_.count(revers_pair_stops) != 0) {
					return length_between_stops_.at(revers_pair_stops);
				}
				return length_between_stops_.at({ lhs , rhs });
				}
			);
		}
	}

	double TransportCatalogue::SummationLineLenght(const Bus& bus) const {
		if (bus.is_loop_trip) {
			return std::transform_reduce(std::next(bus.stops.begin()), bus.stops.end(), bus.stops.begin(), static_cast<double>(0), std::plus<>{}, [](const Stop* lhs, const Stop* rhs) {
				return geo::ComputeDistance(lhs->coordinates, rhs->coordinates);
				}
			);
		}
		else {
			return 2. * std::transform_reduce(std::next(bus.stops.begin()), bus.stops.end(), bus.stops.begin(), static_cast<double>(0), std::plus<>{}, [](const Stop* lhs, const Stop* rhs) {
				return geo::ComputeDistance(lhs->coordinates, rhs->coordinates);
				}
			);
		}
	}

	transport_catalogue_serialize::Stop TransportCatalogue::SaveStopToProto(const Stop& stop) const {
		transport_catalogue_serialize::Stop result;

		transport_catalogue_serialize::Coordinates proto_coord;
		proto_coord.set_lat(stop.coordinates.lat);
		proto_coord.set_lng(stop.coordinates.lng);
		result.set_name(stop.name);
		*result.mutable_coordinates() = proto_coord;

		return result;
	}

	transport_catalogue_serialize::Bus TransportCatalogue::SaveBusToProto(const std::map<std::string_view, size_t>& stops, const Bus& bus) const {
		transport_catalogue_serialize::Bus result;
		result.set_is_loop(bus.is_loop_trip);
		result.set_name(bus.name);
		for (const Stop* stop : bus.stops) {
			result.add_stops(stops.at(stop->name));
		}

		return result;
	}

	transport_catalogue_serialize::Distance TransportCatalogue::SaveLenghtToProto(const std::map<std::string_view, size_t>& stops, const std::pair<Stop*, Stop*>& pair_stops, uint32_t lenght) const {
		transport_catalogue_serialize::Distance result;
		result.set_from(stops.at(pair_stops.first->name));
		result.set_to(stops.at(pair_stops.second->name));
		result.set_lenght(lenght);

		return result;
	}

	transport_catalogue_serialize::TransportCatalogue TransportCatalogue::SaveToProto() const {
		transport_catalogue_serialize::TransportCatalogue result;
		std::map<std::string_view, size_t> proto_stops;
		for (const Stop& stop : all_stops_) {
			proto_stops[stop.name] = result.stops_size();
			*result.add_stops() = SaveStopToProto(stop);
		}
		for (const Bus& bus : all_buses_) {
			*result.add_buses() = SaveBusToProto(proto_stops, bus);
		}
		for (const auto& [pair_stops, lenght] : length_between_stops_) {
			*result.add_lenght_between_stops() = SaveLenghtToProto(proto_stops, pair_stops, lenght);
		}
		return result;
	}

	TransportCatalogue DeserializeTransportCatalogue(const transport_catalogue_serialize::TransportCatalogue& proto_catalogue) {
		TransportCatalogue result;
		for (const auto& proto_stop : proto_catalogue.stops()) {
			result.AddStop(proto_stop.name(), geo::Coordinates{ proto_stop.coordinates().lat(), proto_stop.coordinates().lng() });
		}
		for (const auto& proto_bus : proto_catalogue.buses()) {
			std::vector<std::string_view> stops;
			for (const auto& proto_stop : proto_bus.stops()) {
				stops.push_back(proto_catalogue.stops()[proto_stop].name());
			}
			result.AddBus(proto_bus.name(), std::move(stops), proto_bus.is_loop());
		}
		for (const auto& proto_dist : proto_catalogue.lenght_between_stops()) {
			std::string from = proto_catalogue.stops()[proto_dist.from()].name();
			std::string to = proto_catalogue.stops()[proto_dist.to()].name();
			uint32_t lenght = proto_dist.lenght();
			result.AddLenghtBetweenStops(std::pair<std::string_view, std::string_view>{from, to}, lenght);
		}
		return result;
	}
}