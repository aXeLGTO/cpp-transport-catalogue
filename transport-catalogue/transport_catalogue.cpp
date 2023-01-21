#include "transport_catalogue.h"
#include "domain.h"
#include "geo.h"

#include <numeric>
#include <algorithm>
#include <optional>
#include <unordered_set>
#include <tuple>
#include <utility>

#include <transport_catalogue.pb.h>

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

void TransportCatalogue::SaveTo(std::ostream& output) const {
    unordered_map<StopPtr, int> stop_to_id;

    transport_catalogue_serialize::StopList stop_list;
    for (const auto& stop : GetStopsRange()) {
        transport_catalogue_serialize::Stop stop_raw;
        stop_raw.set_name(stop.name);
        stop_raw.set_lat(stop.coordinates.lat);
        stop_raw.set_lng(stop.coordinates.lng);

        stop_to_id[&stop] = stop_list.stop_size();
        *stop_list.add_stop() = move(stop_raw);
    }

    for (const auto& [stops, distance] : stops_to_distance_) {
        const auto& [from, to] = stops;
        int from_id = stop_to_id[from];
        int to_id = stop_to_id[to];

        auto& distances = *stop_list.mutable_stop(from_id)->mutable_distance();
        distances[to_id] = distance;
    }

    transport_catalogue_serialize::BusList bus_list;
    for (const auto& bus : GetBusesRange()) {
        transport_catalogue_serialize::Bus bus_raw;
        bus_raw.set_name(bus.name);
        bus_raw.set_is_roundtrip(bus.is_roundtrip);

        for (StopPtr stop : bus.stops) {
            bus_raw.add_stop_id(stop_to_id[stop]);
        }

        *bus_list.add_bus() = move(bus_raw);
    }

    transport_catalogue_serialize::Database database;
    *database.mutable_stop_list() = move(stop_list);
    *database.mutable_bus_list() = move(bus_list);

    database.SerializeToOstream(&output);
}

void DeserializeTransportCatalogue(std::istream& input, TransportCatalogue& catalogue) {
    transport_catalogue_serialize::Database database;
    if (!database.ParseFromIstream(&input)) {
        return;
    }

    const auto& stop_list = database.stop_list();
    const auto& bus_list = database.bus_list();

    vector<StopPtr> all_stops;
    all_stops.reserve(stop_list.stop_size());

    for (int stop_id = 0; stop_id < stop_list.stop_size(); ++stop_id) {
        const auto& stop_raw = stop_list.stop(stop_id);

        catalogue.AddStop({stop_raw.name(), {
            stop_raw.lat(),
            stop_raw.lng()
        }});

        all_stops.push_back(&catalogue.FindStop(stop_raw.name()));
    }

    for (int from_id = 0; from_id < stop_list.stop_size(); ++from_id) {
        const auto& from_raw = stop_list.stop(from_id);
        const auto& from = catalogue.FindStop(from_raw.name());

        for (const auto& [to_id, distance] : from_raw.distance()) {
            const auto& to_raw = stop_list.stop(to_id);

            catalogue.SetDistance(
                from,
                catalogue.FindStop(to_raw.name()),
                distance
            );
        }
    }

    for (int bus_id = 0; bus_id < bus_list.bus_size(); ++bus_id) {
        const auto& bus_raw = bus_list.bus(bus_id);

        vector<StopPtr> bus_stops;
        bus_stops.reserve(bus_raw.stop_id_size());

        for (int stop_id = 0; stop_id < bus_raw.stop_id_size(); ++stop_id) {
            bus_stops.push_back(all_stops[bus_raw.stop_id(stop_id)]);
        }

        catalogue.AddBus({
            bus_raw.name(),
            bus_raw.is_roundtrip(),
            move(bus_stops)
        });
    }
}

} // namespace transport_catalogue
