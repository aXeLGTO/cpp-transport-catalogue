#include "transport_catalogue.h"
#include "geo.h"
#include "tests.h"

#include <numeric>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <tuple>
#include <utility>

using namespace std;

namespace transport_catalogue {

void TransportCatalogue::AddStop(const Stop& stop) {
    stops.push_back(move(stop));
    auto* ptr_stop = &stops.back();
    stop_by_name[ptr_stop->name] = ptr_stop;
    stop_to_busnames.insert({ptr_stop, {}});
}

const Stop& TransportCatalogue::FindStop(string_view name) const {
    return *stop_by_name.at(name);
}

void TransportCatalogue::AddBus(const Bus& bus) {
    buses.push_back(move(bus));
    const auto* ptr_bus = &buses.back();

    bus_by_name[ptr_bus->name] = ptr_bus;
    for (const auto* stop : ptr_bus->stops) {
        stop_to_busnames[stop].insert(ptr_bus->name);
    }
}

const Bus& TransportCatalogue::FindBus(string_view name) const {
    return *bus_by_name.at(name);
}

optional<StopInfo> TransportCatalogue::GetStopInfo(string_view name) const {
    if (stop_by_name.count(name) == 0) {
        return nullopt;
    }

    return optional<StopInfo>{{stop_to_busnames.at(&FindStop(name))}};
}

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view name) const {
    if (bus_by_name.count(name) == 0) {
        return nullopt;
    }

    const auto& stops = FindBus(name).stops;
    unordered_set<const Stop*> unique_stops(stops.begin(), stops.end());

    auto coord_distance = transform_reduce(
            next(stops.begin()), stops.end(),
            stops.begin(),
            0.0,
            plus<>{},
            [](const auto* curr, const auto* prev){
                return ComputeDistance(prev->coordinates, curr->coordinates);
        });
    auto distance = transform_reduce(
            next(stops.begin()), stops.end(),
            stops.begin(),
            0.0,
            plus<>{},
            [this](const auto* curr, const auto* prev){
                return GetDistance(*prev, *curr);
        });

    return optional<BusInfo>{{stops.size(), unique_stops.size(), distance, distance / coord_distance}};
}

void TransportCatalogue::SetDistance(const Stop& from, const Stop& to, double distance) {
    stops_to_distance[make_pair(&from, &to)] = distance;
}

double TransportCatalogue::GetDistance(const Stop& from, const Stop& to) const {
    auto stops_pair = make_pair(&from, &to);
    if (stops_to_distance.count(stops_pair) == 0) {
        return stops_to_distance.at(make_pair(&to, &from));
    }
    return stops_to_distance.at(stops_pair);
}

} // namespace transport_catalogue
