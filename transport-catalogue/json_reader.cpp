#include "json_reader.h"
#include "json.h"
#include "request_handler.h"
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

void ReadRequests(TransportCatalogue& catalogue, istream& in, ostream& out) {
    const auto document = Load(in);
    const auto root = document.GetRoot().AsMap();

    ParseInputRequests(catalogue, root.at("base_requests"s).AsArray());
    ParseOutputRequests(RequestHandler{catalogue}, root.at("stat_requests"s).AsArray(), out);
}

void ParseInputRequests(TransportCatalogue& catalogue, const Array& requests) {
    for (const auto& req : requests) {
        const auto type = req.AsMap().at("type"s).AsString();

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
            ParseInputDistanceRequest(catalogue, req);
        } else if (type == "Bus"s) {
            ParseInputBusRequest(catalogue, req);
        }
    }
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
    auto stops = dict.at("stops"s).AsArray();
    bool is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    vector<StopPtr> out_stops;
    ParseRoute(catalogue, stops.begin(), stops.end(), out_stops, is_roundtrip);
    catalogue.AddBus({dict.at("name"s).AsString(), move(out_stops)});
}

void ParseRoute(TransportCatalogue &catalogue, Array::const_iterator first, Array::const_iterator last, std::vector<StopPtr> &out_stops, bool is_roundtrip) {
    if (first == last) {
        return;
    }

    auto* stop = &catalogue.FindStop(first->AsString());
    out_stops.push_back(stop);
    ParseRoute(catalogue, next(first), last, out_stops, is_roundtrip);

    if (!is_roundtrip && next(first) != last) {
        out_stops.push_back(stop);
    }
}

void ParseOutputRequests(const RequestHandler& req_handler, const Array& requests, ostream& out) {
    Array responses;
    for (const auto& req : requests) {
        const auto type = req.AsMap().at("type"s).AsString();
        if (type == "Stop"s) {
            responses.push_back(ParseOutputStopRequest(req_handler, req));
        } else if (type == "Bus"s) {
            responses.push_back(ParseOutputBusRequest(req_handler, req));
        }
    }

    Print(Document{{responses}}, out);
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

} // namespace transport_catalogue

