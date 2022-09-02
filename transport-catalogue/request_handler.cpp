#include "request_handler.h"

#include <numeric>

using namespace std;

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */
namespace transport_catalogue {

RequestHandler::RequestHandler(const TransportCatalogue& db/* , const renderer::MapRenderer& renderer */) :
    db_(db) {
}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    if (const auto bus = db_.FindBus(bus_name)) {
        const auto& stops = bus->stops;
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

} // namespace transport_catalogue {
