#include "transport_catalogue.h"
#include "domain.h"
#include "geo.h"

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
    stop_to_buses.insert({ptr_stop, {}});
}

const Stop& TransportCatalogue::FindStop(string_view name) const {
    return *stop_by_name.at(name);
}

void TransportCatalogue::AddBus(const Bus& bus) {
    buses.push_back(move(bus));
    const auto* ptr_bus = &buses.back();

    bus_by_name[ptr_bus->name] = ptr_bus;
    for (const auto* stop : ptr_bus->stops) {
        stop_to_buses[stop].insert(ptr_bus);
    }
}

BusPtr TransportCatalogue::FindBus(string_view name) const {
    if (bus_by_name.count(name) == 0) {
        return nullptr;
    }
    return bus_by_name.at(name);
}

const std::unordered_set<BusPtr>* TransportCatalogue::GetBusesByStop(string_view name) const {
    if (stop_by_name.count(name) == 0) {
        return nullptr;
    }

    return &stop_to_buses.at(&FindStop(name));
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
