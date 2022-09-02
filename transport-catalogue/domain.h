#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <set>
#include <vector>
/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

using StopPtr = const Stop*;

struct Bus {
    std::string name;
    std::vector<StopPtr> stops;
};

using BusPtr = const Bus*;

struct BusStat {
    size_t stops_amount;
    size_t unique_stops_amount;
    double route_length;
    double curvature;
};

} // namespace transport_catalogue
