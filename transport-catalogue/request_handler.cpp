#include "request_handler.h"
#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "svg.h"

#include <algorithm>
#include <numeric>
#include <set>
#include <vector>

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */
namespace transport_catalogue {

using namespace std;
using namespace renderer;

RequestHandler::RequestHandler(const TransportCatalogue& db, const MapRenderer& renderer) :
    db_(db),
    renderer_(renderer) {
}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    if (const auto bus = db_.FindBus(bus_name)) {
        const vector<StopPtr> stops = MakeRoute(bus);
        unordered_set<StopPtr> unique_stops(stops.begin(), stops.end());

        auto coord_distance = transform_reduce(
                next(stops.begin()), stops.end(),
                stops.begin(),
                0.0,
                plus<>{},
                [](const auto* curr, const auto* prev){
                    return geo::ComputeDistance(prev->coordinates, curr->coordinates);
                });
        auto distance = transform_reduce(
                next(stops.begin()), stops.end(),
                stops.begin(),
                0.0,
                plus<>{},
                [this](const auto* curr, const auto* prev){
                    return db_.GetDistance(*prev, *curr);
                });

        return optional<BusStat>{{stops.size(), unique_stops.size(), distance, distance / coord_distance}};
    }

    return nullopt;
}

// Возвращает маршруты, проходящие через
const std::unordered_set<BusPtr>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

svg::Document RequestHandler::RenderMap() const {
    vector<BusPtr> buses;
    buses.reserve(db_.GetBusesCount());
    for (const auto& [_, bus] : db_) {
        if (!bus->stops.empty()) {
            buses.push_back(bus);
        }
    }

    set<StopPtr, StopComparator> stops;
    for (const auto* bus : buses) {
        stops.insert(bus->stops.begin(), bus->stops.end());
    }

    vector<geo::Coordinates> points(stops.size());
    transform(
        stops.begin(), stops.end(),
        points.begin(),
        [](const StopPtr stop){
            return stop->coordinates;
        }
    );

    SphereProjector projector(
        points.begin(), points.end(), renderer_.GetSetings().width,
        renderer_.GetSetings().height, renderer_.GetSetings().padding
    );

    svg::Document document;
    renderer_.RenderRoutes(buses.begin(), buses.end(), projector, document);
    renderer_.RenderStops(stops.begin(), stops.end(), projector, document);
    return document;
}

} // namespace transport_catalogue {
