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
    stops_.push_back(move(stop));

    auto* ptr_stop = &stops_.back();

    stop_by_name_[ptr_stop->name] = ptr_stop;
    stop_to_buses_.insert({ptr_stop, {}});
}

const Stop& TransportCatalogue::FindStop(string_view name) const {
    return *stop_by_name_.at(name);
}

void TransportCatalogue::AddBus(const Bus& bus) {
    buses_.push_back(move(bus));
    const auto* ptr_bus = &buses_.back();

    bus_by_name_[ptr_bus->name] = ptr_bus;
    for (const auto* stop : ptr_bus->stops) {
        stop_to_buses_[stop].insert(ptr_bus);
    }
}

BusPtr TransportCatalogue::FindBus(string_view name) const {
    if (bus_by_name_.count(name) == 0) {
        return nullptr;
    }
    return bus_by_name_.at(name);
}

optional<BusStat> TransportCatalogue::GetBusStat(string_view bus_name) const {
    if (const auto bus = FindBus(bus_name)) {
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
                    return GetDistance(*prev, *curr);
                });

        return optional<BusStat>{{stops.size(), unique_stops.size(), distance, distance / coord_distance}};
    }

    return nullopt;
}

const std::unordered_set<BusPtr>* TransportCatalogue::GetBusesByStop(string_view name) const {
    if (stop_by_name_.count(name) == 0) {
        return nullptr;
    }

    return &stop_to_buses_.at(&FindStop(name));
}

void TransportCatalogue::SetDistance(const Stop& from, const Stop& to, double distance) {
    stops_to_distance_[make_pair(&from, &to)] = distance;
}

double TransportCatalogue::GetDistance(const Stop& from, const Stop& to) const {
    auto stops_pair = make_pair(&from, &to);
    if (stops_to_distance_.count(stops_pair) == 0) {
        return stops_to_distance_.at(make_pair(&to, &from));
    }
    return stops_to_distance_.at(stops_pair);
}

size_t TransportCatalogue::GetBusesCount() const {
    return buses_.size();
}

size_t TransportCatalogue::GetStopsCount() const {
    return stops_.size();
}

ranges::Range<std::deque<Stop>::const_iterator> TransportCatalogue::GetStopsRange() const {
    return ranges::AsRange(stops_);
}

ranges::Range<std::deque<Bus>::const_iterator> TransportCatalogue::GetBusesRange() const {
    return ranges::AsRange(buses_);
}

} // namespace transport_catalogue
