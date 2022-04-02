#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace proto {
	void Serialization(const transportcatalogue::TransportCatalogue& db, const renderer::MapRender& mr, const transport_router::TransportRouter& tr, std::ostream& out);

	transport_router::TransportRouter* Deserialization(transportcatalogue::TransportCatalogue& db, renderer::MapRender& mr, std::istream& in);
}