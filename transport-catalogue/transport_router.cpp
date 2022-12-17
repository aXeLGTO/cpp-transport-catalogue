#include "transport_router.h"
#include "graph.h"
#include "router.h"

#include <utility>

namespace transport_catalogue {

using namespace std;
using namespace graph;

TransportRouter::TransportRouter(RoutingSettings settings, const TransportCatalogue& db) :
    settings_(move(settings)),
    graph_(db.GetStopsCount() * 2) {

    FillGraphWithStops(db);
    FillGraphWithBuses(db);

    router_ = make_unique<graph::Router<double>>(graph_);
}

optional<TransportRouter::RouteResult> TransportRouter::BuildRoute(const Stop& from, const Stop& to) const {
    auto from_id = vertices_by_stop_.at(&from).first;
    auto to_id = vertices_by_stop_.at(&to).first;

    if (auto route = router_->BuildRoute(from_id, to_id)) {
        vector<RouteItemDesc> items;
        items.reserve(route->edges.size());

        for (const auto& edgeId : route->edges) {
            items.push_back(route_items_by_edges_.at(edgeId));
        }

        return make_pair(route->weight, move(items));
    }

    return nullopt;
}

double TransportRouter::GetRoadTime(double distance) const {
    return distance / (1000 * settings_.bus_velocity) * 60;
}

void TransportRouter::FillGraphWithStops(const TransportCatalogue& db) {
    VertexId id{0};

    for (const auto& stop : db.GetStopsRange()) {
        Edge<double> edge{id++, id++, settings_.bus_wait_time};

        vertices_by_stop_.insert({&stop, {edge.from, edge.to}});
        auto edgeId = graph_.AddEdge(edge);

        route_items_by_edges_.insert({edgeId, {
            RouteItemType::Wait,
            stop.name,
            ""s,
            0,
            settings_.bus_wait_time
        }});
    }
}

void TransportRouter::FillGraphWithBuses(const TransportCatalogue& db) {
    for (const auto& bus : db.GetBusesRange()) {
        if (!bus.stops.empty()) {
            const auto& stops = MakeRoute(bus);

            for (auto from = stops.begin(); from != stops.end(); ++from) {
                auto from_id = vertices_by_stop_.at(*from).second;

                double time = 0.0;
                for (auto to = next(from); to != stops.end(); ++to) {
                    auto to_id = vertices_by_stop_.at(*to).first;

                    time += GetRoadTime(db.GetDistance(**prev(to), **to));

                    Edge<double> edge{from_id, to_id, time};
                    auto edgeId = graph_.AddEdge(edge);

                    route_items_by_edges_.insert({edgeId, {
                        RouteItemType::Bus,
                        ""s,
                        bus.name,
                        static_cast<int>(distance(from, to)),
                        edge.weight
                    }});
                }
            }
        }
    }
}

}
