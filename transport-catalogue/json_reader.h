#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "json.h"

#include <iostream>
#include <unordered_set>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport_catalogue {

renderer::RenderSettings ParseRenderSettings(const json::Document& document);

svg::Point ParsePoint(const json::Array& point);
svg::Color ParseColor(const json::Node& color);

void ParseBaseRequests(TransportCatalogue& catalogue, const json::Document& document);

void ParseInputDistanceRequest(TransportCatalogue& catalogue, const json::Node& request);

void ParseInputBusRequest(TransportCatalogue& catalogue, const json::Node& request);

void ParseRoute(TransportCatalogue& catalogue, json::Array::const_iterator first, json::Array::const_iterator last, std::vector<StopPtr>& out_stops, bool is_roundtrip = false);

void ParseStatRequests(const RequestHandler& request_handler, const json::Document& document, std::ostream& out);

json::Node ParseOutputStopRequest(const RequestHandler& request_handler, const json::Node& request);

json::Node ParseOutputBusRequest(const RequestHandler& request_handler, const json::Node& request);

} // namespace transport_catalogue
