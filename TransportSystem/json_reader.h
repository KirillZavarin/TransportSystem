#pragma once

#include <sstream>
#include <exception>
#include <vector>

#include <optional>
#include <fstream>

#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "serialization.h"

namespace readers {

	using namespace transportcatalogue;

	class Reader {
	public:
		Reader(TransportCatalogue& catalogue, renderer::MapRender& map_render);

		void JSON_BaseRequest(std::istream& in);

		void JSON_StatRequest(std::istream& in, std::ostream& out);

	private:
		std::string JSON_Serialization_Settings(const json::Document& document);

		void JSON_ReaderBus(const json::Document& document);

		void JSON_ReaderStop(const json::Document& document);

		transport_router::RoutingSettings JSON_ReaderRoutingSetings(const json::Document& document);

		renderer::MapSettings JSON_ReaderMapSettings(const json::Document& document);

		json::Node JSON_ResponseRequestBus(const BusInfo& bus_info, int id);

		json::Node JSON_ResponseRequestStop(std::string_view stop_name, int id);

		json::Node JSON_ResponseRequesMap(const svg::Document& map, int id);

		json::Node JSON_ResponseRequesRouter(const RequestHandler& request_handler, const std::string& from, const std::string& to, int id);

		svg::Color JSON_ReaderColor(const json::Node& node);

		TransportCatalogue& catalogue_;
		renderer::MapRender& map_render_;
	};
}

namespace creator {
	class Creator {
	public:
		Creator() :reader(catalogue_, map_render_) {
		}

		void InitializeCatalogue(std::istream& input);

		void ExecutingRequests(std::istream& input, std::ostream& output);

	private:
		TransportCatalogue catalogue_ = {};
		renderer::MapRender map_render_ = {};
		readers::Reader reader;
	};
}