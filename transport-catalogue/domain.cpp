#include "domain.h"

/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области
 * (domain) вашего приложения и не зависят от транспортного справочника. Например Автобусные
 * маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */
namespace transport_catalogue {

using namespace std;

bool StopComparator::operator()(StopPtr lhs, StopPtr rhs) const {
    return lhs->name < rhs->name;
}

vector<StopPtr> MakeRoute(BusPtr bus) {
    std::vector<StopPtr> stops;
    MakeRoute(bus->stops.begin(), bus->stops.end(), stops, bus->is_roundtrip);
    return stops;
}

} // namespace transport_catalogue
