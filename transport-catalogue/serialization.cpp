#include "serialization.h"
#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "graph.h"

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

    auto transport_catalogue = details::Deserialize(database.transport_catalogue());
    auto transport_router = details::Deserialize(database.transport_router(), transport_catalogue);
    auto map_renderer = details::Deserialize(database.map_renderer());

    DeserializeResult result{
        move(transport_catalogue),
        move(map_renderer),
        move(transport_router)
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
    *object.mutable_router() = Serialize(transport_router.GetRouter());
    return object;
}

transport_catalogue::TransportRouter Deserialize(const TransportRouter& object, const transport_catalogue::TransportCatalogue& transport_catalogue) {
    auto router_data = Deserialize(object.router());

    return {
        Deserialize(object.routing_settings()),
        move(router_data),
        transport_catalogue,
    };
}

Stop Serialize(const transport_catalogue::Stop& stop) {
    Stop object;

    object.set_name(stop.name);
    object.set_lat(stop.coordinates.lat);
    object.set_lng(stop.coordinates.lng);

    return object;
}

Bus Serialize(const transport_catalogue::Bus& bus) {
    Bus object;

    object.set_name(bus.name);
    object.set_is_roundtrip(bus.is_roundtrip);

    return object;
}

RenderSettings Serialize(const renderer::RenderSettings& render_settings) {
    RenderSettings object;

    object.set_width(render_settings.width);
    object.set_height(render_settings.height);
    object.set_padding(render_settings.padding);
    object.set_line_width(render_settings.line_width);
    object.set_stop_radius(render_settings.stop_radius);
    object.set_bus_label_font_size(render_settings.bus_label_font_size);
    object.set_stop_label_font_size(render_settings.stop_label_font_size);
    object.set_underlayer_width(render_settings.underlayer_width);

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

    render_settings.width = object.width();
    render_settings.height = object.height();
    render_settings.padding = object.padding();
    render_settings.line_width = object.line_width();
    render_settings.stop_radius = object.stop_radius();
    render_settings.bus_label_font_size = object.bus_label_font_size();
    render_settings.stop_label_font_size = object.stop_label_font_size();
    render_settings.underlayer_width = object.underlayer_width();

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

    object.set_bus_wait_time(routing_settings.bus_wait_time);
    object.set_bus_velocity(routing_settings.bus_velocity);

    return object;
}

transport_catalogue::RoutingSettings Deserialize(const RoutingSettings& object) {
    transport_catalogue::RoutingSettings routing_settings;

    routing_settings.bus_wait_time = object.bus_wait_time();
    routing_settings.bus_velocity = object.bus_velocity();

    return routing_settings;
}

Router Serialize(const transport_catalogue::TransportRouter::Router& router) {
    Router object;

    for (const auto& routes : router.GetRoutesRange()) {
        auto& route_list = *object.add_route_list();

        for (const auto& route : routes) {
            auto& route_object = *route_list.add_route();

            if (route) {
                RouteData data;
                data.set_weight(route->weight);

                if (route->prev_edge) {
                    EdgeId edge_id;
                    edge_id.set_id(*route->prev_edge);
                    *data.mutable_prev_edge() = move(edge_id);
                }

                *route_object.mutable_data() = move(data);
            }
        }
    }

    return object;
}

transport_catalogue::TransportRouter::Router::RoutesInternalData Deserialize(const Router& object) {
    using RouteInternalData = transport_catalogue::TransportRouter::Router::RouteInternalData;
    using RoutesInternalData = transport_catalogue::TransportRouter::Router::RoutesInternalData;

    int vertex_count = object.route_list_size();
    RoutesInternalData data(vertex_count, vector<optional<RouteInternalData>>(vertex_count));

    for (int from_id = 0; from_id < vertex_count; ++from_id) {
        const auto& route_list = object.route_list(from_id);

        for (int to_id = 0; to_id < route_list.route_size(); ++to_id) {
            const auto& route = route_list.route(to_id);

            if (route.has_data()) {
                const auto& route_data = route.data();
                optional<graph::EdgeId> prev_edge;

                if (route_data.has_prev_edge()) {
                    prev_edge = route_data.prev_edge().id();
                }

                data[from_id][to_id] = {
                    route_data.weight(),
                    prev_edge
                };
            }
        }
    }

    return data;
}

Point Serialize(const svg::Point& point) {
    Point object;

    object.set_x(point.x);
    object.set_y(point.y);

    return object;
}

svg::Point Deserialize(const Point& object) {
    return {
        object.x(),
        object.y()
    };
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

    object.set_red(rgb.red);
    object.set_green(rgb.green);
    object.set_blue(rgb.blue);

    return object;
}

Rgba Serialize(const svg::Rgba& rgba) {
    Rgba object;

    object.set_red(rgba.red);
    object.set_green(rgba.green);
    object.set_blue(rgba.blue);
    object.set_opacity(rgba.opacity);

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
