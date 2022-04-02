#pragma once

#include <deque>
#include <string>

#include "geo.h"

namespace transportcatalogue {

	struct Stop {
		std::string name;
		geo::Coordinates coordinates;

		geo::Coordinates GetCoord() const;
	};

	struct Bus {
		std::string name;
		std::deque<Stop*> stops;
		bool is_loop_trip;
	};

	struct StopInfo {
		bool exists;
		std::deque<std::string_view> buses;
	};

	struct BusInfo {
		bool exists;

		double curvature;
		double route_length;

		int quantity_stop;
		int unique_stops;
	};
}