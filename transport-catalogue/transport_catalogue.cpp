#include "transport_catalogue.h"
#include "geo.h"

#include <numeric>
#include <algorithm>
#include <unordered_set>
#include <tuple>
#include <utility>

using namespace std;

namespace transport_catalogue {

auto Stop::MakeTuple() const {
    return tie(name, latitude, longitude);
}

bool Stop::operator==(const Stop& other) const {
    return MakeTuple() == other.MakeTuple();
}

bool Stop::operator!=(const Stop& other) const {
    return !(*this == other);
}

auto Bus::MakeTuple() const {
    return tie(name, stops);
}

bool Bus::operator==(const Bus& other) const {
    return MakeTuple() == other.MakeTuple();
}

bool Bus::operator!=(const Bus& other) const {
    return !(*this == other);
}

auto StopInfo::MakeTuple() const {
    return tie(buses);
}

bool StopInfo::operator==(const StopInfo& other) const {
    return MakeTuple() == other.MakeTuple();
}

bool StopInfo::operator!=(const StopInfo& other) const {
    return !(*this == other);
}


auto BusInfo::MakeTuple() const {
    return tie(stops_amount, unique_stops_amount, route_length);
}

bool BusInfo::operator==(const BusInfo& other) const {
    return MakeTuple() == other.MakeTuple();
}

bool BusInfo::operator!=(const BusInfo& other) const {
    return !(*this == other);
}

void TransportCatalogue::AddStop(Stop stop) {
    stops.push_back(move(stop));
    auto* ptr_stop = &stops.back();
    stopname_to_stop[ptr_stop->name] = ptr_stop;
    stop_to_busnames.insert({ptr_stop, {}});
}

const Stop& TransportCatalogue::FindStop(string_view name) const {
    return *stopname_to_stop.at(name);
}

void TransportCatalogue::AddBus(Bus bus) {
    buses.push_back(move(bus));
    const auto* ptr_bus = &buses.back();

    busname_to_bus[ptr_bus->name] = ptr_bus;
    for (const auto* stop : ptr_bus->stops) {
        stop_to_busnames[stop].insert(ptr_bus->name);
    }
}

const Bus& TransportCatalogue::FindBus(string_view name) const {
    return *busname_to_bus.at(name);
}

tuple<bool, StopInfo> TransportCatalogue::GetStopInfo(string_view name) const {
    if (stopname_to_stop.count(name) == 0) {
        return make_tuple(false, StopInfo{});
    }

    const auto& stop = FindStop(name);
    return make_tuple(true, StopInfo{stop_to_busnames.at(&stop)});
}

tuple<bool, BusInfo> TransportCatalogue::GetBusInfo(string_view name) const {
    if (busname_to_bus.count(name) == 0) {
        return {false, {}};
    }

    const auto& stops = FindBus(name).stops;
    unordered_set<const Stop*> unique_stops(stops.begin(), stops.end());

    auto coord_distance = transform_reduce(
            next(stops.begin()), stops.end(),
            stops.begin(),
            0.0,
            plus<>{},
            [](const auto* curr, const auto* prev){
                return ComputeDistance(Coordinates{prev->latitude, prev->longitude}, Coordinates{curr->latitude, curr->longitude});
        });
    auto distance = transform_reduce(
            next(stops.begin()), stops.end(),
            stops.begin(),
            0.0,
            plus<>{},
            [this](const auto* curr, const auto* prev){
                return GetDistance(make_pair(prev, curr));
        });

    return make_tuple(true, BusInfo{stops.size(), unique_stops.size(), distance, distance / coord_distance});
}

void TransportCatalogue::AddDistance(StopsPair stops_pair, double distance) {
    stops_to_distance[stops_pair] = distance;
}

double TransportCatalogue::GetDistance(StopsPair stops_pair) const {
    if (stops_to_distance.count(stops_pair) == 0) {
        return stops_to_distance.at({stops_pair.second, stops_pair.first});
    }
    return stops_to_distance.at(stops_pair);
}

} // namespace transport_catalogue
