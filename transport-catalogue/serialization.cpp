#include "serialization.h"
#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

using namespace std;

namespace transport_catalogue_serialize {

void Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue,
               const renderer::MapRenderer& map_renderer,
               const transport_catalogue::TransportRouter& transport_router,
               std::ostream &output) {
    Database database;
    *database.mutable_transport_catalogue() = details::Serialize(transport_catalogue);
    *database.mutable_map_renderer() = details::Serialize(map_renderer);
    *database.mutable_transport_router() = details::Serialize(transport_router);
    database.SerializeToOstream(&output);
}

optional<DeserializeResult> Deserialize(istream& input) {
    using namespace transport_catalogue;

    Database database;
    if (!database.ParseFromIstream(&input)) {
        return nullopt;
    }

    DeserializeResult result{
        details::Deserialize(database.transport_catalogue()),
        details::Deserialize(database.map_renderer()),
        details::Deserialize(database.transport_router())
    };

    return {move(result)};
}

namespace details {

TransportCatalogue Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue) {
    unordered_map<transport_catalogue::StopPtr, int> stop_to_id;

    StopList stop_list;
    for (const auto& stop : transport_catalogue.GetStopsRange()) {
        stop_to_id[&stop] = stop_list.stop_size();
        *stop_list.add_stop() = Serialize(stop);
    }

    for (const auto& [stops, distance] : transport_catalogue.GetStopsDistanceRange()) {
        const auto& [from, to] = stops;
        int from_id = stop_to_id[from];
        int to_id = stop_to_id[to];

        auto& distances = *stop_list.mutable_stop(from_id)->mutable_distance();
        distances[to_id] = distance;
    }

    BusList bus_list;
    for (const auto& bus : transport_catalogue.GetBusesRange()) {
        auto object = Serialize(bus);
        for (auto* stop : bus.stops) {
            object.add_stop_id(stop_to_id[stop]);
        }
        *bus_list.add_bus() = move(object);
    }

    TransportCatalogue object;
    *object.mutable_stop_list() = move(stop_list);
    *object.mutable_bus_list() = move(bus_list);
    return object;
}

transport_catalogue::TransportCatalogue Deserialize(const TransportCatalogue& object) {
    transport_catalogue::TransportCatalogue transport_catalogue;
    const auto& stop_list = object.stop_list();
    vector<transport_catalogue::StopPtr> all_stops;
    all_stops.reserve(stop_list.stop_size());

    for (int stop_id = 0; stop_id < stop_list.stop_size(); ++stop_id) {
        const auto& stop_raw = stop_list.stop(stop_id);

        transport_catalogue.AddStop({stop_raw.name(), {
            stop_raw.lat(),
            stop_raw.lng()
        }});

        all_stops.push_back(&transport_catalogue.FindStop(stop_raw.name()));
    }

    for (int from_id = 0; from_id < stop_list.stop_size(); ++from_id) {
        const auto& stop = stop_list.stop(from_id);
        const auto& from = transport_catalogue.FindStop(stop.name());

        for (const auto& [to_id, distance] : stop.distance()) {
            transport_catalogue.SetDistance(
                from,
                transport_catalogue.FindStop(stop_list.stop(to_id).name()),
                distance
            );
        }
    }

    const auto& bus_list = object.bus_list();

    for (int bus_id = 0; bus_id < bus_list.bus_size(); ++bus_id) {
        const auto& bus = bus_list.bus(bus_id);

        vector<transport_catalogue::StopPtr> bus_stops;
        bus_stops.reserve(bus.stop_id_size());

        for (int stop_id = 0; stop_id < bus.stop_id_size(); ++stop_id) {
            bus_stops.push_back(all_stops[bus.stop_id(stop_id)]);
        }

        transport_catalogue.AddBus({
            bus.name(),
            bus.is_roundtrip(),
            move(bus_stops)
        });
    }

    return transport_catalogue;
}

MapRenderer Serialize(const renderer::MapRenderer& map_renderer) {
    MapRenderer object;
    *object.mutable_render_settings() = Serialize(map_renderer.GetSetings());
    return object;
}

renderer::MapRenderer Deserialize(const MapRenderer& object) {
    return renderer::MapRenderer(Deserialize(object.render_settings()));
}

TransportRouter Serialize(const transport_catalogue::TransportRouter& transport_router) {
    TransportRouter object;
    *object.mutable_routing_settings() = Serialize(transport_router.GetSettings());
    return object;
}

transport_catalogue::TransportRouter Deserialize(const TransportRouter& object) {
    return transport_catalogue::TransportRouter(Deserialize(object.routing_settings()));
}

Stop Serialize(const transport_catalogue::Stop& stop) {
    Stop object;

    SET_X(stop, object, name);
    SET_X(stop.coordinates, object, lat);
    SET_X(stop.coordinates, object, lng);

    return object;
}

Bus Serialize(const transport_catalogue::Bus& bus) {
    Bus object;

    SET_X(bus, object, name);
    SET_X(bus, object, is_roundtrip);

    return object;
}

RenderSettings Serialize(const renderer::RenderSettings& render_settings) {
    RenderSettings object;

    SET_X(render_settings, object, width);
    SET_X(render_settings, object, height);
    SET_X(render_settings, object, padding);
    SET_X(render_settings, object, line_width);
    SET_X(render_settings, object, stop_radius);
    SET_X(render_settings, object, bus_label_font_size);
    SET_X(render_settings, object, stop_label_font_size);
    SET_X(render_settings, object, underlayer_width);

    *object.mutable_bus_label_offset() = Serialize(render_settings.bus_label_offset);
    *object.mutable_stop_label_offset() = Serialize(render_settings.stop_label_offset);
    *object.mutable_underlayer_color() = Serialize(render_settings.underlayer_color);

    for (const auto& color : render_settings.color_palette) {
        *object.add_color_palette() = Serialize(color);
    }

    return object;
}

renderer::RenderSettings Deserialize(const RenderSettings& object) {
    renderer::RenderSettings render_settings;

    GET_X(object, render_settings, width);
    GET_X(object, render_settings, height);
    GET_X(object, render_settings, padding);
    GET_X(object, render_settings, line_width);
    GET_X(object, render_settings, stop_radius);
    GET_X(object, render_settings, bus_label_font_size);
    GET_X(object, render_settings, stop_label_font_size);
    GET_X(object, render_settings, underlayer_width);

    render_settings.bus_label_offset = Deserialize(object.bus_label_offset());
    render_settings.stop_label_offset = Deserialize(object.stop_label_offset());
    render_settings.underlayer_color = Deserialize(object.underlayer_color());

    vector<svg::Color> color_palette;
    color_palette.reserve(object.color_palette_size());
    for (int i = 0; i < object.color_palette_size(); ++i) {
        color_palette.push_back(Deserialize(object.color_palette(i)));
    }
    render_settings.color_palette = move(color_palette);

    return render_settings;
}

RoutingSettings Serialize(const transport_catalogue::RoutingSettings& routing_settings) {
    RoutingSettings object;

    SET_X(routing_settings, object, bus_wait_time);
    SET_X(routing_settings, object, bus_velocity);

    return object;
}

transport_catalogue::RoutingSettings Deserialize(const RoutingSettings& object) {
    transport_catalogue::RoutingSettings routing_settings;

    GET_X(object, routing_settings, bus_wait_time);
    GET_X(object, routing_settings, bus_velocity);

    return routing_settings;
}

Point Serialize(const svg::Point& point) {
    Point object;

    SET_X(point, object, x);
    SET_X(point, object, y);

    return object;
}

svg::Point Deserialize(const Point& object) {
    svg::Point point;

    GET_X(object, point, x);
    GET_X(object, point, y);

    return point;
}

Color Serialize(const svg::Color& color) {
    Color object;

    if (holds_alternative<string>(color)) {
        object.set_name(get<string>(color));
    } else if (holds_alternative<svg::Rgb>(color)) {
        *object.mutable_rgb() = Serialize(get<svg::Rgb>(color));
    } else {
        *object.mutable_rgba() = Serialize(get<svg::Rgba>(color));
    }

    return object;
}

svg::Color Deserialize(const Color& object) {
    if (object.has_rgba()) {
        return Deserialize(object.rgba());
    } else if (object.has_rgb()) {
        return Deserialize(object.rgb());
    }

    return object.name();
}

Rgb Serialize(const svg::Rgb& rgb) {
    Rgb object;

    SET_X(rgb, object, red);
    SET_X(rgb, object, green);
    SET_X(rgb, object, blue);

    return object;
}

Rgba Serialize(const svg::Rgba& rgba) {
    Rgba object;

    SET_X(rgba, object, red);
    SET_X(rgba, object, green);
    SET_X(rgba, object, blue);
    SET_X(rgba, object, opacity);

    return object;
}

svg::Rgba Deserialize(const Rgba& object) {
    return {
        static_cast<uint8_t>(object.red()),
        static_cast<uint8_t>(object.green()),
        static_cast<uint8_t>(object.blue()),
        object.opacity()
    };
}

svg::Rgb Deserialize(const Rgb& object) {
    return {
        static_cast<uint8_t>(object.red()),
        static_cast<uint8_t>(object.green()),
        static_cast<uint8_t>(object.blue())
    };
}

} // namespace details

} // namespace transport_catalogue_serialize
