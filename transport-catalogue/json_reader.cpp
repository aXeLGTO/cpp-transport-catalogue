#include "json_reader.h"
#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <unordered_map>
#include <vector>
#include <set>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport_catalogue {

using namespace std;
using namespace json;
using namespace renderer;

RenderSettings ParseRenderSettings(const json::Document& document) {
    const auto& settings = document.GetRoot().AsMap().at("render_settings"s).AsMap();
    vector<svg::Color> color_palette;
    for (const auto& color : settings.at("color_palette"s).AsArray()) {
        color_palette.push_back(details::ParseColor(color));
    }
    return {
        settings.at("width"s).AsDouble(),
        settings.at("height"s).AsDouble(),
        settings.at("padding"s).AsDouble(),
        settings.at("line_width"s).AsDouble(),
        settings.at("stop_radius"s).AsDouble(),
        settings.at("bus_label_font_size"s).AsInt(),
        details::ParsePoint(settings.at("bus_label_offset"s).AsArray()),
        settings.at("stop_label_font_size"s).AsInt(),
        details::ParsePoint(settings.at("stop_label_offset"s).AsArray()),
        details::ParseColor(settings.at("underlayer_color"s)),
        settings.at("underlayer_width"s).AsDouble(),
        move(color_palette)
    };
}

void ParseBaseRequests(TransportCatalogue& catalogue, const json::Document& document) {
    const auto requests = document.GetRoot().AsMap().at("base_requests"s).AsArray();
    for (const auto& req : requests) {
        const auto& type = req.AsMap().at("type"s).AsString();

        if (type == "Stop"s) {
            catalogue.AddStop({req.AsMap().at("name"s).AsString(), {
                req.AsMap().at("latitude"s).AsDouble(),
                req.AsMap().at("longitude"s).AsDouble()
            }});
        }
    }

    for (const auto& req : requests) {
        const auto type = req.AsMap().at("type"s).AsString();

        if (type == "Stop"s) {
            details::ParseInputDistanceRequest(catalogue, req);
        } else if (type == "Bus"s) {
            details::ParseInputBusRequest(catalogue, req);
        }
    }
}

void ParseStatRequests(const RequestHandler& req_handler, const Document& document, ostream& out) {
    Array responses;
    for (const auto& req : document.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
        const auto type = req.AsMap().at("type"s).AsString();
        if (type == "Stop"s) {
            responses.push_back(details::ParseOutputStopRequest(req_handler, req));
        } else if (type == "Bus"s) {
            responses.push_back(details::ParseOutputBusRequest(req_handler, req));
        }
    }

    Print(json::Document{{responses}}, out);
}

namespace details {

svg::Point ParsePoint(const json::Array &point) {
    return {point[0].AsDouble(), point[1].AsDouble()};
}

svg::Color ParseColor(const json::Node& color) {
    if (color.IsArray()) {
        const auto& rgba = color.AsArray();
        if (rgba.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(rgba[0].AsInt()),
                static_cast<uint8_t>(rgba[1].AsInt()),
                static_cast<uint8_t>(rgba[2].AsInt()),
                rgba[3].AsDouble(),
            };
        }
        return svg::Rgb{
            static_cast<uint8_t>(rgba[0].AsInt()),
            static_cast<uint8_t>(rgba[1].AsInt()),
            static_cast<uint8_t>(rgba[2].AsInt()),
        };
    }
    return color.AsString();
}

void ParseInputDistanceRequest(TransportCatalogue& catalogue, const Node& request) {
    const auto from = request.AsMap().at("name"s).AsString();
    for (const auto& [to, distance] : request.AsMap().at("road_distances"s).AsMap()) {
        catalogue.SetDistance(
            catalogue.FindStop(from),
            catalogue.FindStop(to),
            distance.AsDouble());
    }
}

void ParseInputBusRequest(TransportCatalogue& catalogue, const Node& request) {
    auto dict = request.AsMap();
    bool is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    vector<StopPtr> stops;
    for (const auto& stop_name : dict.at("stops"s).AsArray()) {
        stops.push_back(&catalogue.FindStop(stop_name.AsString()));
    }

    catalogue.AddBus({dict.at("name"s).AsString(), is_roundtrip, move(stops)});
}

Node ParseOutputStopRequest(const RequestHandler& req_handler, const Node& req) {
    if (const auto* buses = req_handler.GetBusesByStop(req.AsMap().at("name"s).AsString())) {
        set<string> bus_names;
        for (const auto* bus : *buses) {
            bus_names.insert(bus->name);
        }
        return {{
            {"request_id"s, {req.AsMap().at("id"s).AsInt()}},
            {"buses"s, {Array{bus_names.begin(), bus_names.end()}}}
        }};
    } else {
        return {{
            {"request_id"s, {req.AsMap().at("id"s).AsInt()}},
            {"error_message"s, {"not found"s}}
        }};
    }
}

Node ParseOutputBusRequest(const RequestHandler& req_handler, const Node& req) {
    if (const auto bus_stat = req_handler.GetBusStat(req.AsMap().at("name"s).AsString())) {
        return {{
            {"request_id"s, {req.AsMap().at("id"s).AsInt()}},
            {"curvature"s, {bus_stat->curvature}},
            {"route_length"s, {bus_stat->route_length}},
            {"stop_count"s, {static_cast<int>(bus_stat->stops_amount)}},
            {"unique_stop_count"s, {static_cast<int>(bus_stat->unique_stops_amount)}},
        }};
    } else {
        return {{
            {"request_id"s, {req.AsMap().at("id"s).AsInt()}},
            {"error_message"s, {"not found"s}}
        }};
    }
}

} // namespace details

} // namespace transport_catalogue

