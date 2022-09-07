#include "tests.h"
#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "svg.h"
#include "test_framework.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;
using namespace transport_catalogue;

bool operator==(const Stop& lhs, const Stop& rhs) {
    if (lhs.name == rhs.name) {
        return abs(lhs.coordinates.lat - rhs.coordinates.lat) < TOLERANCE
            && abs(lhs.coordinates.lng - rhs.coordinates.lng) < TOLERANCE;
    }
    return false;
}

bool operator!=(const Stop& lhs, const Stop& rhs) {
    return !(lhs == rhs);
}

bool operator==(const Bus& lhs, const Bus& rhs) {
    return tie(lhs.name, lhs.stops) == tie(rhs.name, rhs.stops);
}

bool operator!=(const Bus& lhs, const Bus& rhs) {
    return !(lhs == rhs);
}

bool operator==(const BusStat& lhs, const BusStat& rhs) {
    if (tie(lhs.stops_amount, lhs.unique_stops_amount) ==
        tie(rhs.stops_amount, rhs.unique_stops_amount)) {
        return abs(lhs.route_length - rhs.route_length) < TOLERANCE;
    }

    return false;
}

bool operator!=(const BusStat& lhs, const BusStat& rhs) {
    return !(lhs == rhs);
}

ostream& operator<<(ostream& out, const Stop& stop) {
    return out << "("s << stop.name << ", "s << stop.coordinates.lat << ", "s << stop.coordinates.lng << ")"s;
}

ostream& operator<<(ostream& out, const Bus& bus) {
    return out << "("s << bus.name << ", "s << bus.stops << ")"s;
}

ostream& operator<<(ostream& out, const BusStat& bus_info) {
    return out << "("s
        << bus_info.stops_amount << ", "s
        << bus_info.unique_stops_amount << ", "s
        << bus_info.route_length << ", "s
        << bus_info.curvature << ")"s;
}

bool operator==(const svg::Point& lhs, const svg::Point& rhs) {
    return tie(lhs.x, lhs.y) == tie(rhs.x, rhs.y);
}

bool operator!=(const svg::Point& lhs, const svg::Point& rhs) {
    return !(lhs == rhs);
}

// bool operator==(const renderer::RenderSettings& lhs, const renderer::RenderSettings& rhs) {
//     return tie(lhs.width, lhs.height, lhs.padding, lhs.line_width, lhs.stop_radius,
//                lhs.bus_label_font_size, lhs.bus_label_offset, lhs.stop_label_font_size,
//                lhs.stop_label_offset, lhs.underlayer_color, lhs.underlayer_width,
//                lhs.color_palette) ==
//            tie(rhs.width, rhs.height, rhs.padding, rhs.line_width, rhs.stop_radius,
//                rhs.bus_label_font_size, rhs.bus_label_offset, rhs.stop_label_font_size,
//                rhs.stop_label_offset, rhs.underlayer_color, rhs.underlayer_width,
//                rhs.color_palette);
// }
//
// bool operator!=(const renderer::RenderSettings& lhs, const renderer::RenderSettings& rhs) {
//     return !(lhs == rhs);
// }

void TestAddStop() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        ASSERT_EQUAL(catalogue.FindStop(stop1.name), stop1);
    }
}

void TestAddBus() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);

        Bus bus1{"256"s, false, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name)
        }};
        catalogue.AddBus(bus1);
        ASSERT_EQUAL(*catalogue.FindBus("256"s), bus1);
    }
}

void TestGetStopInfo() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    {
        TransportCatalogue catalogue;
        const auto* empty = catalogue.GetBusesByStop("256"s);
        ASSERT(!empty);
    }
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        Bus bus1{"750"s, false, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name)
        }};
        Bus bus2{"256"s, false, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus1);
        catalogue.AddBus(bus2);

        unordered_map<string_view, set<string_view>> stop_to_busnames{
            {stop1.name, {bus1.name, bus2.name}},
            {stop2.name, {bus1.name, bus2.name}},
            {stop3.name, {bus2.name}}
        };
        for (const auto& [stop_name, busnames] : stop_to_busnames) {
            const auto* busesPtr = catalogue.GetBusesByStop(stop_name);
            ASSERT(busesPtr);
            vector<BusPtr> buses(busesPtr->begin(), busesPtr->end());
            for (const auto& name : busnames) {
                ASSERT(find_if(buses.begin(), buses.end(), [&name](const auto* v){return v->name == name;}) != buses.end());
            }
        }
    }
}

void TestGetBusInfo() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    double distance_stop1_to_stop2 = 3900;
    double distance_stop2_to_stop3 = 9900;
    double distance_stop2_to_stop2 = 100;
    double distance_stop3_to_stop2 = 9500;
    using namespace geo;
    auto coord_distance = ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng})
        + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
        + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
        + ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng});
    auto distance = distance_stop1_to_stop2 + distance_stop2_to_stop2 + distance_stop2_to_stop3 + distance_stop3_to_stop2 + distance_stop2_to_stop2 + distance_stop1_to_stop2;

    {
        RequestHandler request_handler{TransportCatalogue{}, renderer::MapRenderer{{}}};
        const auto empty = request_handler.GetBusStat("256"s);
        ASSERT(!empty);
    }

    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop1.name),
            catalogue.FindStop(stop2.name),
        distance_stop1_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop3.name),
        distance_stop2_to_stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop2.name),
        distance_stop2_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop3.name),
            catalogue.FindStop(stop2.name),
        distance_stop3_to_stop2);

        Bus bus{"750"s, false, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
        }};
        catalogue.AddBus(bus);

        RequestHandler request_handler{catalogue, renderer::MapRenderer{{}}};
        auto bus_info = request_handler.GetBusStat(bus.name);
        ASSERT(bus_info);
        ASSERT_EQUAL(bus_info->stops_amount, 7u);
        ASSERT_EQUAL(bus_info->unique_stops_amount, 3u);
        ASSERT(abs(bus_info->route_length - distance) < TOLERANCE);
        ASSERT(abs(bus_info->curvature - (distance / coord_distance)) < TOLERANCE);
    }
}

void TestAddStopsDistance() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    double distance_stop1_to_stop2 = 3900;
    double distance_stop2_to_stop3 = 9900;
    double distance_stop2_to_stop2 = 100;
    double distance_stop3_to_stop2 = 9500;
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop1.name),
            catalogue.FindStop(stop2.name),
            distance_stop1_to_stop2
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop3.name),
            distance_stop2_to_stop3
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop2.name),
            distance_stop2_to_stop2
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop3.name),
            catalogue.FindStop(stop2.name),
            distance_stop3_to_stop2
        );

        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop1.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop1_to_stop2);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop2.name),
                    catalogue.FindStop(stop3.name)),
                distance_stop2_to_stop3);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop2.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop2_to_stop2);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop3.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop3_to_stop2);

        ASSERT_EQUAL(catalogue.GetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop1.name)
        ), distance_stop1_to_stop2);
    }
}

void TestParsePoint() {
    const json::Array& raw_point{{15.0}, {-37.5}};
    const svg::Point& point = details::ParsePoint(raw_point);
    ASSERT(abs(point.x - raw_point[0].AsDouble()) < TOLERANCE);
    ASSERT(abs(point.y - raw_point[1].AsDouble()) < TOLERANCE);
}

void TestParseColor() {
    const json::Node& color_str = {"red"s};
    const svg::Color& color_name = details::ParseColor(color_str);
    ASSERT_EQUAL(color_str.AsString(), get<string>(color_name));

    const json::Array& color_rgb = {{0}, {150}, {255}};
    const svg::Rgb rgb = get<svg::Rgb>(details::ParseColor(color_rgb));
    ASSERT_EQUAL(color_rgb[0].AsInt(), rgb.red);
    ASSERT_EQUAL(color_rgb[1].AsInt(), rgb.green);
    ASSERT_EQUAL(color_rgb[2].AsInt(), rgb.blue);

    const json::Array& color_rgba = {{0}, {150}, {255}, {0.25}};
    const svg::Rgba rgba = get<svg::Rgba>(details::ParseColor(color_rgba));
    ASSERT_EQUAL(color_rgba[0].AsInt(), rgba.red);
    ASSERT_EQUAL(color_rgba[1].AsInt(), rgba.green);
    ASSERT_EQUAL(color_rgba[2].AsInt(), rgba.blue);
    ASSERT(abs(color_rgba[3].AsDouble() - rgba.opacity) < TOLERANCE);
}

// void TestParseRenderSettings() {
//     using namespace json;
//     svg::Color color = svg::Color{svg::Rgba{255, 255, 255, 0.85}};
//     renderer::RenderSettings render_settings{
//         600, 400, 50, 14, 5,
//         20, {7, 15},
//         20, {7, -3},
//         svg::Color{svg::Rgba{255, 255, 255, 0.85}}, 3,
//         {
//             svg::Color{"green"s},
//             svg::Color{svg::Rgb{255, 255, 255}}
//         }
//     };
//
//     const Document document{{Dict{
//         {"width"s, {render_settings.width}},
//         {"height"s, {render_settings.height}},
//         {"padding"s, {render_settings.padding}},
//         {"line_width"s, {render_settings.line_width}},
//         {"stop_radius"s, {render_settings.stop_radius}},
//         {"bus_label_font_size"s, {render_settings.bus_label_font_size}},
//         {"bus_label_offset"s, {Array{
//             {render_settings.bus_label_offset.x},
//             {render_settings.bus_label_offset.y}
//         }}},
//         {"stop_label_font_size"s, {render_settings.stop_label_font_size}},
//         {"stop_label_offset"s, {Array{
//             {render_settings.stop_label_offset.x},
//             {render_settings.stop_label_offset.y}
//         }}},
//         {"underlayer_color"s, {Array{
//             {get<svg::Rgba>(render_settings.underlayer_color).red},
//             {get<svg::Rgba>(render_settings.underlayer_color).green},
//             {get<svg::Rgba>(render_settings.underlayer_color).blue},
//             {get<svg::Rgba>(render_settings.underlayer_color).opacity}
//         }}},
//         {"underlayer_width"s, {render_settings.underlayer_width}},
//         {"color_palette"s, {Array{
//             {get<string>(render_settings.color_palette[0])},
//             {Array{
//                 {get<svg::Rgba>(render_settings.underlayer_color).red},
//                 {get<svg::Rgba>(render_settings.underlayer_color).green},
//                 {get<svg::Rgba>(render_settings.underlayer_color).blue}
//             }}
//         }}}
//     }}};
//
//     ASSERT_EQUAL(ParseRenderSettings(document), render_settings);
// }

void TestAll() {
    RUN_TEST(TestAddStop);
    RUN_TEST(TestAddBus);
    RUN_TEST(TestGetBusInfo);
    RUN_TEST(TestGetStopInfo);
    RUN_TEST(TestAddStopsDistance);

    RUN_TEST(TestParsePoint);
    RUN_TEST(TestParseColor);
    //RUN_TEST(TestParseRenderSettings);

    cerr << "All tests done"s << endl;
}

