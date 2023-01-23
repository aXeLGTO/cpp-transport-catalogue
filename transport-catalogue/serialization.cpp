#include "serialization.h"
#include "domain.h"

using namespace std;

namespace transport_catalogue_serialize {

void SerializeTransportCatalogue(const transport_catalogue::TransportCatalogue &catalogue, const renderer::RenderSettings& settings, std::ostream &output) {
    using namespace transport_catalogue;

    unordered_map<StopPtr, int> stop_to_id;

    StopList stop_list;
    for (const auto& stop : catalogue.GetStopsRange()) {
        Stop stop_raw;

        SET_X(stop, stop_raw, name);
        SET_X(stop.coordinates, stop_raw, lat);
        SET_X(stop.coordinates, stop_raw, lng);

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

    BusList bus_list;
    for (const auto& bus : catalogue.GetBusesRange()) {
        Bus bus_raw;

        SET_X(bus, bus_raw, name);
        SET_X(bus, bus_raw, is_roundtrip);

        for (StopPtr stop : bus.stops) {
            bus_raw.add_stop_id(stop_to_id[stop]);
        }

        *bus_list.add_bus() = move(bus_raw);
    }

    Database database;
    *database.mutable_stop_list() = move(stop_list);
    *database.mutable_bus_list() = move(bus_list);
    *database.mutable_render_settings() = details::SerializeRenderSetings(settings);
    database.SerializeToOstream(&output);
}

bool DeserializeTransportCatalogue(istream& input, transport_catalogue::TransportCatalogue& catalogue, renderer::RenderSettings& render_settings) {
    using namespace transport_catalogue;

    Database database;
    if (!database.ParseFromIstream(&input)) {
        return false;
    }

    const auto& stop_list = database.stop_list();
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

    const auto& bus_list = database.bus_list();

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

    details::DeserializeRenderSetings(database.render_settings(), render_settings);

    return true;
}

namespace details {

RenderSettings SerializeRenderSetings(const renderer::RenderSettings& settings) {
    RenderSettings settings_raw;

    SET_X(settings, settings_raw, width);
    SET_X(settings, settings_raw, height);
    SET_X(settings, settings_raw, padding);
    SET_X(settings, settings_raw, line_width);
    SET_X(settings, settings_raw, stop_radius);
    SET_X(settings, settings_raw, bus_label_font_size);
    SET_X(settings, settings_raw, stop_label_font_size);
    SET_X(settings, settings_raw, underlayer_width);

    *settings_raw.mutable_bus_label_offset() = SerializePoint(settings.bus_label_offset);
    *settings_raw.mutable_stop_label_offset() = SerializePoint(settings.stop_label_offset);
    *settings_raw.mutable_underlayer_color() = SerializeColor(settings.underlayer_color);

    for (const auto& color : settings.color_palette) {
        *settings_raw.add_color_palette() = SerializeColor(color);
    }

    return settings_raw;
}

void DeserializeRenderSetings(const RenderSettings& settings_raw, renderer::RenderSettings& render_settings) {
    GET_X(settings_raw, render_settings, width);
    GET_X(settings_raw, render_settings, height);
    GET_X(settings_raw, render_settings, padding);
    GET_X(settings_raw, render_settings, line_width);
    GET_X(settings_raw, render_settings, stop_radius);
    GET_X(settings_raw, render_settings, bus_label_font_size);
    GET_X(settings_raw, render_settings, stop_label_font_size);
    GET_X(settings_raw, render_settings, underlayer_width);

    render_settings.bus_label_offset = DeserializePoint(settings_raw.bus_label_offset());
    render_settings.stop_label_offset = DeserializePoint(settings_raw.stop_label_offset());
    render_settings.underlayer_color = DeserializeColor(settings_raw.underlayer_color());

    vector<svg::Color> color_palette;
    color_palette.reserve(settings_raw.color_palette_size());
    for (int i = 0; i < settings_raw.color_palette_size(); ++i) {
        color_palette.push_back(DeserializeColor(settings_raw.color_palette(i)));
    }
    render_settings.color_palette = move(color_palette);
}

Point SerializePoint(const svg::Point& point) {
    Point point_raw;

    SET_X(point, point_raw, x);
    SET_X(point, point_raw, y);

    return point_raw;
}

svg::Point DeserializePoint(const Point& point_raw) {
    svg::Point point;

    GET_X(point_raw, point, x);
    GET_X(point_raw, point, y);

    return point;
}

Color SerializeColor(const svg::Color& color) {
    Color color_raw;

    if (holds_alternative<string>(color)) {
        color_raw.set_name(get<string>(color));
    } else if (holds_alternative<svg::Rgb>(color)) {
        *color_raw.mutable_rgb() = SerializeRgb(get<svg::Rgb>(color));
    } else {
        *color_raw.mutable_rgba() = SerializeRgba(get<svg::Rgba>(color));
    }

    return color_raw;
}

svg::Color DeserializeColor(const Color& color) {
    if (color.has_rgba()) {
        return DeserializeRgba(color.rgba());
    } else if (color.has_rgb()) {
        return DeserializeRgb(color.rgb());
    }

    return color.name();
}

Rgb SerializeRgb(const svg::Rgb& rgb) {
    Rgb rgb_raw;

    SET_X(rgb, rgb_raw, red);
    SET_X(rgb, rgb_raw, green);
    SET_X(rgb, rgb_raw, blue);

    return rgb_raw;
}

Rgba SerializeRgba(const svg::Rgba& rgba) {
    Rgba rgba_raw;

    SET_X(rgba, rgba_raw, red);
    SET_X(rgba, rgba_raw, green);
    SET_X(rgba, rgba_raw, blue);
    SET_X(rgba, rgba_raw, opacity);

    return rgba_raw;
}

svg::Rgba DeserializeRgba(const Rgba& rgba) {
    return {
        static_cast<uint8_t>(rgba.red()),
        static_cast<uint8_t>(rgba.green()),
        static_cast<uint8_t>(rgba.blue()),
        rgba.opacity()
    };
}

svg::Rgb DeserializeRgb(const Rgb& rgb) {
    return {
        static_cast<uint8_t>(rgb.red()),
        static_cast<uint8_t>(rgb.green()),
        static_cast<uint8_t>(rgb.blue())
    };
}

} // namespace details

} // namespace transport_catalogue_serialize
