#include "json_reader.h"

using namespace std::string_literals;

namespace creator {
	void Creator::InitializeCatalogue(std::istream& input) {
		reader.JSON_BaseRequest(input);
	}

	void Creator::ExecutingRequests(std::istream& input, std::ostream& output) {
		reader.JSON_StatRequest(input, output);
	}
}

namespace readers {

	using namespace transportcatalogue;

	Reader::Reader(TransportCatalogue& catalogue, renderer::MapRender& map_render) : catalogue_(catalogue), map_render_(map_render) {
	}

	std::string Reader::JSON_Serialization_Settings(const json::Document& document) {
		return document.GetRoot().AsDict().at("file"s).AsString();
	}

	void Reader::JSON_BaseRequest(std::istream& in) {
		json::Document query = json::Load(in);

		std::ofstream ofile(JSON_Serialization_Settings(json::Document(query.GetRoot().AsDict().at("serialization_settings"s))), std::ios::binary);

		json::Document document(query.GetRoot().AsDict().at("base_requests"s));
		for (const auto& description : document.GetRoot().AsArray()) {
			if (description.AsDict().find("type") == description.AsDict().end()) {
				throw  std::invalid_argument("key not found: type");
			}

			if (description.AsDict().at("type"s) == "Bus"s) {
				JSON_ReaderBus(json::Document(description));
			}
		}

		for (const auto& description : document.GetRoot().AsArray()) {
			if (description.AsDict().at("type"s) == "Stop"s) {
				JSON_ReaderStop(json::Document(description));
			}
		}

		std::shared_ptr<const transport_router::TransportRouter> router(new transport_router::TransportRouter(JSON_ReaderRoutingSetings(json::Document(query.GetRoot().AsDict().at("routing_settings"s))), catalogue_));

		renderer::MapRender map_render(JSON_ReaderMapSettings(json::Document(query.GetRoot().AsDict().at("render_settings"s))));

		proto::Serialization(catalogue_, map_render, *router, ofile);
	}

	void Reader::JSON_ReaderBus(const json::Document& document) {
		if (document.GetRoot().AsDict().find("name") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: name");
		}
		if (document.GetRoot().AsDict().find("stops") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: stops");
		}
		if (document.GetRoot().AsDict().find("is_roundtrip") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: is_roundtrip");
		}
		std::string name_bus = document.GetRoot().AsDict().at("name").AsString();
		bool is_loop = document.GetRoot().AsDict().at("is_roundtrip").AsBool();
		std::vector<std::string_view> stop_names;
		for (const auto& stop : document.GetRoot().AsDict().at("stops").AsArray()) {
			stop_names.push_back(stop.AsString());
		}
		catalogue_.AddBus(name_bus, std::move(stop_names), is_loop);
	}

	void Reader::JSON_ReaderStop(const json::Document& document) {
		if (document.GetRoot().AsDict().find("name") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: name");
		}
		if (document.GetRoot().AsDict().find("latitude") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: latitude");
		}
		if (document.GetRoot().AsDict().find("longitude") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: longitude");
		}
		if (document.GetRoot().AsDict().find("road_distances") == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: road_distances");
		}
		std::string name_stop = document.GetRoot().AsDict().at("name").AsString();
		double latitude = document.GetRoot().AsDict().at("latitude").AsDouble();
		double longitude = document.GetRoot().AsDict().at("longitude").AsDouble();
		auto map_lenght = document.GetRoot().AsDict().at("road_distances").AsDict();

		catalogue_.AddStop(name_stop, geo::Coordinates{ latitude, longitude });

		std::pair<std::string, std::string> pair_stop;
		pair_stop.first = name_stop;

		for (const auto& [name_second_stop, lenght] : map_lenght) {
			pair_stop.second = name_second_stop;
			catalogue_.AddLenghtBetweenStops(pair_stop, std::abs(lenght.AsInt()));
		}
	}

	void Reader::JSON_StatRequest(std::istream& in, std::ostream& out) {
		json::Document query = json::Load(in);
		std::ifstream ifile(JSON_Serialization_Settings(json::Document(query.GetRoot().AsDict().at("serialization_settings"s))), std::ios::binary);
		transport_router::TransportRouter* router_ptr = proto::Deserialization(catalogue_, map_render_, ifile);

		//std::cout << router_ptr->router_ptr_->graph_.GetVertexCount() << std::endl;
		//std::cout << router_ptr->graph_of_stops.GetVertexCount() << std::endl;
		//setlocale(LC_ALL, "RU");
		//for (const auto& el : router_ptr->GetRouteInfo("??????? ??????"s, "???????????? ?????"s).value().GetItems()) {
		//	std::cout << el.name_ << std::endl;
		//}

		std::shared_ptr<transport_router::TransportRouter> router(router_ptr);
		RequestHandler request_handler(catalogue_, map_render_, router);
		json::Document document(query.GetRoot().AsDict().at("stat_requests"s));

		json::Array result;

		for (const auto& request : document.GetRoot().AsArray()) {
			if (request.AsDict().find("type"s) == request.AsDict().end()) {
				throw std::invalid_argument("key not found: type"s);
			}
			if (request.AsDict().find("id"s) == request.AsDict().end()) {
				throw std::invalid_argument("key not found: id"s);
			}

			if (request.AsDict().at("type"s).AsString() == "Bus"s) {
				if (request.AsDict().find("name"s) == request.AsDict().end()) {
					throw std::invalid_argument("key not found: name"s);
				}
				result.push_back(JSON_ResponseRequestBus(request_handler.GetBusStat(request.AsDict().at("name"s).AsString()), request.AsDict().at("id"s).AsInt()));  //? request_handler
			}
			if (request.AsDict().at("type"s).AsString() == "Stop"s) {
				if (request.AsDict().find("name"s) == request.AsDict().end()) {
					throw std::invalid_argument("key not found: name"s);
				}
				result.push_back(JSON_ResponseRequestStop(request.AsDict().at("name"s).AsString(), request.AsDict().at("id"s).AsInt()));
			}
			if (request.AsDict().at("type"s).AsString() == "Map"s) {
				result.push_back(JSON_ResponseRequesMap(request_handler.RenderMap(), request.AsDict().at("id"s).AsInt()));
			}
			if (request.AsDict().at("type"s).AsString() == "Route"s) {
				result.push_back(JSON_ResponseRequesRouter(request_handler, request.AsDict().at("from"s).AsString(), request.AsDict().at("to"s).AsString(), request.AsDict().at("id"s).AsInt()));
			}

		}
		Print(json::Document{ json::Node {result} }, out);
	}

	json::Node Reader::JSON_ResponseRequestBus(const BusInfo& bus_info, int id) {
		if (bus_info.exists) {
			return json::Builder()
				.StartDict()
				.Key("curvature"s).Value(bus_info.curvature)
				.Key("request_id"s).Value(id)
				.Key("route_length"s).Value(bus_info.route_length)
				.Key("stop_count"s).Value(bus_info.quantity_stop)
				.Key("unique_stop_count"s).Value(bus_info.unique_stops)
				.EndDict().Build();
		}
		else {
			return json::Builder()
				.StartDict()
				.Key("request_id"s).Value(id)
				.Key("error_message"s).Value("not found"s)
				.EndDict().Build();
		}
	}

	json::Node Reader::JSON_ResponseRequestStop(std::string_view stop_name, int id) {
		auto stop_info = catalogue_.GetInfoStop(stop_name);
		if (stop_info.exists) {
			json::Builder builder;
			builder.StartDict().Key("buses"s).StartArray();
			for (const auto& bus : stop_info.buses) {
				builder.Value(std::string(bus));
			}
			builder.EndArray();
			builder.Key("request_id"s).Value(id).EndDict();
			return builder.Build();
		}
		else {
			return json::Builder{}.StartDict()
				.Key("error_message"s).Value("not found"s)
				.Key("request_id"s).Value(id)
				.EndDict()
				.Build();
		}
	}

	json::Node Reader::JSON_ResponseRequesMap(const svg::Document& map, int id) {
		json::Node id_node(id);
		std::ostringstream stream;
		map.Render(stream);

		std::ofstream map_file("map.svg");

		map_file << stream.str();

		json::Node render_node(stream.str());
		return json::Node{ json::Dict{ { "map", render_node }, { "request_id", id_node } } };
	}

	json::Node Reader::JSON_ResponseRequesRouter(const RequestHandler& request_handler, const std::string& from, const std::string& to, int id) {
		std::optional<transport_router::RouteInfo> info_ort = request_handler.GetRouteStat(from, to);

		if (!info_ort) {
			return json::Builder().StartDict().Key("error_message"s).Value("not found"s).Key("request_id"s).Value(id).EndDict().Build();
		}

		transport_router::RouteInfo info = info_ort.value();

		json::Builder builder;
		builder.StartDict().Key("items"s).StartArray();
		for (const auto& item : info.GetItems()) {
			if (item.span_count_) {
				builder.StartDict().
					Key("bus"s).Value(std::string(item.name_)).
					Key("time"s).Value(item.weight_).
					Key("type"s).Value("Bus"s).
					Key("span_count"s).Value(static_cast<int>(item.span_count_.value()))
					.EndDict();
			}
			else {
				builder.StartDict().
					Key("stop_name"s).Value(std::string(item.name_)).
					Key("time"s).Value(item.weight_).
					Key("type"s).Value("Wait"s)
					.EndDict();
			}
		}
		builder.EndArray();
		return builder.Key("request_id"s).Value(id)
			.Key("total_time"s).Value(info.GetTotalTime()).EndDict().Build();
	}

	transport_router::RoutingSettings Reader::JSON_ReaderRoutingSetings(const json::Document& document) {
		if (document.GetRoot().AsDict().find("bus_wait_time"s) == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: bus wait time"s);
		}
		if (document.GetRoot().AsDict().find("bus_velocity"s) == document.GetRoot().AsDict().end()) {
			throw std::invalid_argument("key not found: bus velocity"s);
		}
		return { static_cast<unsigned short int>(document.GetRoot().AsDict().at("bus_wait_time"s).AsInt()),
				 static_cast<unsigned short int>(document.GetRoot().AsDict().at("bus_velocity"s).AsInt()) };
	}

	renderer::MapSettings Reader::JSON_ReaderMapSettings(const json::Document& document) {
		json::Dict map_query = document.GetRoot().AsDict();

		double width = map_query.at("width").AsDouble();
		double height = map_query.at("height").AsDouble();
		double padding = map_query.at("padding").AsDouble();
		double line_width = map_query.at("line_width").AsDouble();
		double stop_radius = map_query.at("stop_radius").AsDouble();
		int bus_label_font_size = map_query.at("bus_label_font_size").AsInt();
		json::Array arr_bus_label_offset = map_query.at("bus_label_offset").AsArray();
		double bus_label_offset[2];
		for (int i = 0; i < 2; i++) {
			bus_label_offset[i] = arr_bus_label_offset[i].AsDouble();
		}
		int stop_label_font_size = map_query.at("stop_label_font_size").AsInt();
		json::Array arr_stop_label_offset = map_query.at("stop_label_offset").AsArray();
		double stop_label_offset[2];
		for (int i = 0; i < 2; i++) {
			stop_label_offset[i] = arr_stop_label_offset[i].AsDouble();
		}
		double underlayer_width = map_query.at("underlayer_width").AsDouble();
		svg::Color underlayer_color = JSON_ReaderColor(map_query.at("underlayer_color"));
		json::Array arr_color_palette = map_query.at("color_palette").AsArray();

		std::vector<svg::Color> color_palette;

		for (const auto& color : arr_color_palette) {
			color_palette.push_back(JSON_ReaderColor(color));
		}

		renderer::MapSettings result({ width, height, padding },
			{ line_width, stop_radius },
			{ bus_label_font_size, bus_label_offset },
			{ stop_label_font_size, stop_label_offset },
			underlayer_color, underlayer_width, color_palette);

		return result;
	}

	svg::Color Reader::JSON_ReaderColor(const json::Node& node) {
		if (node.IsString()) {
			return svg::Color(node.AsString());
		}
		else {
			svg::Color result;
			std::vector<uint8_t> color;
			if (node.AsArray().size() == 4) {
				result = svg::Rgba(node.AsArray()[0].AsInt(), node.AsArray()[1].AsInt(), node.AsArray()[2].AsInt(), node.AsArray()[3].AsDouble());
			}
			else {
				result = svg::Rgb(node.AsArray()[0].AsInt(), node.AsArray()[1].AsInt(), node.AsArray()[2].AsInt());
			}
			return result;
		}
	}
}