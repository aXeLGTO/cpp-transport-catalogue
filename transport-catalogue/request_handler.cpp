#include "request_handler.h"
#include "domain.h"
#include "map_renderer.h"
#include "svg.h"

#include <vector>

namespace transport_catalogue {

using namespace std;
using namespace renderer;

RequestHandler::RequestHandler(const TransportCatalogue& db, const MapRenderer& renderer) :
    db_(db),
    renderer_(renderer) {
}

std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusStat(bus_name);
}

const std::unordered_set<BusPtr>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    vector<BusPtr> buses;
    for (const auto& [_, bus] : db_) {
        if (!bus->stops.empty()) {
            buses.push_back(bus);
        }
    }

    return renderer_.RenderMap(buses.begin(), buses.end());
}

} // namespace transport_catalogue {
