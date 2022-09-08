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

void ParseBaseRequests(TransportCatalogue& catalogue, const json::Document& document);

void ParseStatRequests(const RequestHandler& request_handler, const json::Document& document, std::ostream& out);

namespace details {

svg::Point ParsePoint(const json::Array& point);

svg::Color ParseColor(const json::Node& color);

void ParseInputDistanceRequest(TransportCatalogue& catalogue, const json::Node& request);

void ParseInputBusRequest(TransportCatalogue& catalogue, const json::Node& request);

json::Node ParseOutputStopRequest(const RequestHandler& request_handler, const json::Node& request);

json::Node ParseOutputBusRequest(const RequestHandler& request_handler, const json::Node& request);

json::Node ParseOutputMapRequest(const RequestHandler& req_handler, const json::Node& req);

} // namespace details

} // namespace transport_catalogue
