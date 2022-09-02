#pragma once
#include "transport_catalogue.h"
#include "request_handler.h"
#include "json.h"

#include <iostream>
#include <unordered_set>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport_catalogue {

void ReadRequests(TransportCatalogue& catalogue, std::istream& in, std::ostream& out);

void ParseInputRequests(TransportCatalogue& catalogue, const json::Array& requests);

void ParseInputDistanceRequest(TransportCatalogue& catalogue, const json::Node& request);

void ParseInputBusRequest(TransportCatalogue& catalogue, const json::Node& request);

void ParseRoute(TransportCatalogue& catalogue, json::Array::const_iterator first, json::Array::const_iterator last, std::vector<StopPtr>& out_stops, bool is_roundtrip = false);

void ParseOutputRequests(const RequestHandler& request_handler, const json::Array& requests, std::ostream& out);

json::Node ParseOutputStopRequest(const RequestHandler& request_handler, const json::Node& request);

json::Node ParseOutputBusRequest(const RequestHandler& request_handler, const json::Node& request);

} // namespace transport_catalogue
