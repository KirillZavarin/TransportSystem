#pragma once
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
#include <unordered_set>

#include <transport_catalogue.pb.h>

#include "domain.h"

using namespace std::string_literals;

namespace transportcatalogue {

	class TransportCatalogue {
	public:

		using BusPtr = Bus*;
		using StopPtr = Stop*;

		const std::unordered_map<std::string_view, Bus*> GetBuses() const {
			return buses_;
		}

		const std::deque<Stop>& GetAllStops() const {
			return all_stops_;
		}

		const std::deque<Bus>& GetAllBuses() const {
			return all_buses_;
		}

		const Bus* GetBus(std::string_view name_bus) const {
			return buses_.at(name_bus);
		}

		const Stop* GetStop(std::string_view stop_bus) const {
			return stops_.at(stop_bus);
		}

		bool StopAvailability(std::string_view stop) const {
			return stops_.count(stop) != 0;
		}

		bool BusAvailability(std::string_view bus) const {
			return buses_.count(bus) != 0;
		}

		void AddBus(const std::string_view bus, std::vector<std::string_view>&& stops, bool is_loop);

		void AddStop(const std::string_view, const geo::Coordinates& location = { 0., 0. });

		void AddLenghtBetweenStops(const std::pair<std::string_view, std::string_view>&, uint32_t lenght);

		[[nodiscard]] const BusInfo GetInfoBus(std::string_view bus_name) const;

		[[nodiscard]] const StopInfo GetInfoStop(std::string_view stop_name) const;

		Bus& FindBus(const std::string_view name) const {
			return *buses_.at(name);
		}

		Stop& FindStop(const std::string_view name) const {
			return *stops_.at(name);
		}

		uint32_t GetLenghtBetweenStops(std::string_view first_stop, std::string_view second_stop) const {
			std::pair<Stop*, Stop*> pair_stop = { stops_.at(first_stop), stops_.at(second_stop) };
			auto it = length_between_stops_.find(pair_stop);
			if (it == length_between_stops_.end()) {
				it = length_between_stops_.find(std::pair{ pair_stop.second, pair_stop.first });
				if (it == length_between_stops_.end()) {
					if (first_stop == second_stop) {
						return 0;
					}
					else {
						throw std::logic_error("Not found Lenght between this stops");
					}
				}
			}
			return it->second;
		}

		transport_catalogue_serialize::TransportCatalogue SaveToProto() const;

	private:

		transport_catalogue_serialize::Stop SaveStopToProto(const Stop& stop) const;

		transport_catalogue_serialize::Bus SaveBusToProto(const std::map<std::string_view, size_t>& stops, const Bus& bus) const;

		transport_catalogue_serialize::Distance SaveLenghtToProto(const std::map<std::string_view, size_t>& stops, const std::pair<Stop*, Stop*>& pair_stops, uint32_t lenght) const;

		struct PairStopHasher {
			size_t  operator()(const std::pair<Stop*, Stop*>& pair_stop) const {
				size_t h_stop1 = ptr_hasher(pair_stop.first);
				size_t h_stop2 = ptr_hasher(pair_stop.second);
				return 47 * h_stop1 + h_stop2;
			}
			std::hash<const void*> ptr_hasher;
		};

		double SummationLineLenght(const Bus& bus) const;

		double SummationLenght(const Bus& bus) const;

		std::unordered_map<std::string_view, Bus*> buses_;
		std::unordered_map<std::string_view, Stop*> stops_;


		std::unordered_map<std::string_view, std::deque<std::string_view>> buses_in_stop_;

		std::unordered_map<std::pair<Stop*, Stop*>, uint32_t, PairStopHasher> length_between_stops_;

		std::deque<Bus> all_buses_;
		std::deque<Stop> all_stops_;
	};

	TransportCatalogue DeserializeTransportCatalogue(const transport_catalogue_serialize::TransportCatalogue& proto_catalogue);
}