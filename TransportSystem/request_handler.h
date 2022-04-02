#pragma once

#include <unordered_set>
#include <optional>
#include <set>
#include <string_view>
#include <algorithm>
#include <utility>
#include <optional>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace transportcatalogue;

class RequestHandler {
public:
    // MapRenderer ïîíàäîáèòñÿ â ñëåäóþùåé ÷àñòè èòîãîâîãî ïðîåêòà
    RequestHandler(const TransportCatalogue& db, const renderer::MapRender& map_setings, const std::shared_ptr<const transport_router::TransportRouter> router);

    // Âîçâðàùàåò èíôîðìàöèþ î ìàðøðóòå (çàïðîñ Bus)
    BusInfo GetBusStat(std::string_view bus_name) const;

    // Ýòîò ìåòîä áóäåò íóæåí â ñëåäóþùåé ÷àñòè èòîãîâîãî ïðîåêòà
    svg::Document RenderMap() const;

    std::optional<transport_router::RouteInfo> GetRouteStat(const std::string& from, const std::string& to) const;

private:
    const TransportCatalogue& db_;
    const renderer::MapRender& render_;
    std::shared_ptr<const transport_router::TransportRouter> router_;
};