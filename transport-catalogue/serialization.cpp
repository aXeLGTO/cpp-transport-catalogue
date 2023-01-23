#include "serialization.h"
#include "domain.h"

#include <transport_catalogue.pb.h>

using namespace std;

namespace transport_catalogue_serialize {

void SerializeTransportCatalogue(const transport_catalogue::TransportCatalogue &catalogue, std::ostream &output) {
    using namespace transport_catalogue;

    unordered_map<StopPtr, int> stop_to_id;

    transport_catalogue_serialize::StopList stop_list;
    for (const auto& stop : catalogue.GetStopsRange()) {
        transport_catalogue_serialize::Stop stop_raw;
        stop_raw.set_name(stop.name);
        stop_raw.set_lat(stop.coordinates.lat);
        stop_raw.set_lng(stop.coordinates.lng);

        stop_to_id[&stop] = stop_list.stop_size();
        *stop_list.add_stop() = move(stop_raw);
    }

    for (const auto& [stops, distance] : catalogue.GetStopsDistanceRange()) {
        const auto& [from, to] = stops;
        int from_id = stop_to_id[from];
        int to_id = stop_to_id[to];

        auto& distances = *stop_list.mutable_stop(from_id)->mutable_distance();
        distances[to_id] = distance;
    }

    transport_catalogue_serialize::BusList bus_list;
    for (const auto& bus : catalogue.GetBusesRange()) {
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

transport_catalogue::TransportCatalogue DeserializeTransportCatalogue(istream& input) {
    using namespace transport_catalogue;

    TransportCatalogue catalogue;

    transport_catalogue_serialize::Database database;
    if (!database.ParseFromIstream(&input)) {
        return catalogue;
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

    return catalogue;
}

} // namespace transport_catalogue_serialize
