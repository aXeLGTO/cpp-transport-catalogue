#pragma once
#include "domain.h"
#include "transport_catalogue.h"
#include "router.h"

#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace transport_catalogue {

struct SerializationSettings {
    std::string file;
};

struct RoutingSettings {
    double bus_wait_time;
    double bus_velocity;
};

enum class RouteItemType {
    Wait,
    Bus
};

struct RouteItemDesc {
    RouteItemType type;
    std::string stop_name;
    std::string bus_name;
    int span_count;
    double time;
};

class TransportRouter {
public:
    using RouteResult = std::pair<double, std::vector<RouteItemDesc>>;

    TransportRouter(RoutingSettings settings, const TransportCatalogue& db);

    std::optional<RouteResult> BuildRoute(const Stop& from, const Stop& to) const;

private:
    void FillGraphWithStops(const TransportCatalogue& db);

    void FillGraphWithBuses(const TransportCatalogue& db);

    double GetRoadTime(double distance) const;

    const RoutingSettings& settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;

    std::unordered_map<StopPtr, std::pair<graph::VertexId, graph::VertexId>> vertices_by_stop_;
    std::unordered_map<graph::EdgeId, RouteItemDesc> route_items_by_edges_;
};

}
