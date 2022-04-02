#include "serialization.h"

namespace proto {
	void Serialization(const transportcatalogue::TransportCatalogue& db, const renderer::MapRender& mr, const transport_router::TransportRouter& tr, std::ostream& out) {
		transport_catalogue_serialize::Common result;
		*result.mutable_catalogue() = db.SaveToProto();
		*result.mutable_map_settings() = mr.SaveToProto();
		*result.mutable_router() = tr.SaveToProto();
		result.SerializeToOstream(&out);
	}

	transport_router::TransportRouter* Deserialization(transportcatalogue::TransportCatalogue& db, renderer::MapRender& mr, std::istream& in) {
		transport_catalogue_serialize::Common result;
		if (result.ParseFromIstream(&in)) {
			db = transportcatalogue::DeserializeTransportCatalogue(result.catalogue());
			mr = renderer::DeserializeMapRender(result.map_settings());
			return transport_router::DeserializeTransportRouter(result.router(), db);
		}
		return nullptr;
	}
}