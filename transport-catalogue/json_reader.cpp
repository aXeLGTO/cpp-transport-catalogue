#include "json_reader.h"
#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "svg.h"
#include "transport_catalogue.h"

#include <sstream>
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
    const auto& settings = document.GetRoot().AsDict().at("render_settings"s).AsDict();
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

transport_catalogue::RoutingSettings ParseRoutingSettings(const json::Document& document) {
    const auto& settings = document.GetRoot().AsDict().at("routing_settings"s).AsDict();
    return {
        settings.at("bus_wait_time"s).AsDouble(),
        settings.at("bus_velocity"s).AsDouble(),
    };
}

void ParseBaseRequests(TransportCatalogue& catalogue, const json::Document& document) {
    const auto requests = document.GetRoot().AsDict().at("base_requests"s).AsArray();
    for (const auto& req : requests) {
        const auto& type = req.AsDict().at("type"s).AsString();

        if (type == "Stop"s) {
            catalogue.AddStop({req.AsDict().at("name"s).AsString(), {
                req.AsDict().at("latitude"s).AsDouble(),
                req.AsDict().at("longitude"s).AsDouble()
            }});
        }
    }

    for (const auto& req : requests) {
        const auto type = req.AsDict().at("type"s).AsString();

        if (type == "Stop"s) {
            details::ParseInputDistanceRequest(catalogue, req);
        } else if (type == "Bus"s) {
            details::ParseInputBusRequest(catalogue, req);
        }
    }
}

void ParseStatRequests(const RequestHandler& req_handler, const Document& document, ostream& out) {
    Array responses;
    for (const auto& req : document.GetRoot().AsDict().at("stat_requests"s).AsArray()) {
        const auto type = req.AsDict().at("type"s).AsString();
        if (type == "Stop"s) {
            responses.push_back(details::ParseOutputStopRequest(req_handler, req));
        } else if (type == "Bus"s) {
            responses.push_back(details::ParseOutputBusRequest(req_handler, req));
        } else if (type == "Map"s) {
            responses.push_back(details::ParseOutputMapRequest(req_handler, req));
        } else if (type == "Route"s) {
            responses.push_back(details::ParseOutputRouteRequest(req_handler, req));
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
    const auto from = request.AsDict().at("name"s).AsString();
    for (const auto& [to, distance] : request.AsDict().at("road_distances"s).AsDict()) {
        catalogue.SetDistance(
            catalogue.FindStop(from),
            catalogue.FindStop(to),
            distance.AsDouble());
    }
}

void ParseInputBusRequest(TransportCatalogue& catalogue, const Node& request) {
    auto dict = request.AsDict();
    bool is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    vector<StopPtr> stops;
    for (const auto& stop_name : dict.at("stops"s).AsArray()) {
        stops.push_back(&catalogue.FindStop(stop_name.AsString()));
    }

    catalogue.AddBus({dict.at("name"s).AsString(), is_roundtrip, move(stops)});
}

Node ParseOutputStopRequest(const RequestHandler& req_handler, const Node& req) {
    if (const auto* buses = req_handler.GetBusesByStop(req.AsDict().at("name"s).AsString())) {
        set<string> bus_names;
        for (const auto* bus : *buses) {
            bus_names.insert(bus->name);
        }
        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("buses"s).Value(Array{bus_names.begin(), bus_names.end()})
            .EndDict()
            .Build();
    } else {
        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build();
    }
}

Node ParseOutputBusRequest(const RequestHandler& req_handler, const Node& req) {
    if (const auto bus_stat = req_handler.GetBusStat(req.AsDict().at("name"s).AsString())) {
        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("curvature"s).Value(bus_stat->curvature)
                .Key("route_length"s).Value(bus_stat->route_length)
                .Key("stop_count"s).Value(static_cast<int>(bus_stat->stops_amount))
                .Key("unique_stop_count"s).Value(static_cast<int>(bus_stat->unique_stops_amount))
            .EndDict()
            .Build();
    } else {
        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build();
    }
}

Node ParseOutputMapRequest(const RequestHandler& req_handler, const Node& req) {
    ostringstream out;
    req_handler.RenderMap().Render(out);

    return Builder{}
        .StartDict()
            .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
            .Key("map"s).Value(out.str())
        .EndDict()
        .Build();
}

Node ParseOutputRouteRequest(const RequestHandler& req_handler, const Node& req) {
    const auto from = req.AsDict().at("from"s).AsString();
    const auto to = req.AsDict().at("to"s).AsString();

    if (auto result = req_handler.BuildRoute(from, to)) {
        const auto [total_time, route_items] = *result;

        Builder itemsBuilder;
        auto arrayBuilder = itemsBuilder.StartArray();
        for (const auto& item : route_items) {
            if (item.type == RouteItemType::Wait) {
                arrayBuilder.Value(Builder{}
                    .StartDict()
                        .Key("type"s).Value("Wait"s)
                        .Key("stop_name"s).Value(item.stop_name)
                        .Key("time"s).Value(item.time)
                    .EndDict()
                    .Build()
                    .AsDict()
                );
            } else {
                arrayBuilder.Value(Builder{}
                    .StartDict()
                        .Key("type"s).Value("Bus"s)
                        .Key("bus"s).Value(item.bus_name)
                        .Key("span_count"s).Value(item.span_count)
                        .Key("time"s).Value(item.time)
                    .EndDict()
                    .Build()
                    .AsDict()
                );
            }
        }
        arrayBuilder.EndArray();

        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("total_time"s).Value(total_time)
                .Key("items"s).Value(itemsBuilder.Build().AsArray())
            .EndDict()
            .Build();
    } else {
        return Builder{}
            .StartDict()
                .Key("request_id"s).Value(req.AsDict().at("id"s).AsInt())
                .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build();
    }
}

} // namespace details

} // namespace transport_catalogue

