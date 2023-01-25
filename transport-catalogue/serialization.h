#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "svg.h"

#include <iostream>
#include <optional>
#include <transport_catalogue.pb.h>

#define SET_X(src, dst, field)    \
    dst.set_##field(src.field)

#define GET_X(src, dst, field)    \
    dst.field = src.field()

namespace transport_catalogue_serialize {

struct DeserializeResult {
    transport_catalogue::TransportCatalogue transport_catalogue;
    renderer::MapRenderer map_renderer;
    transport_catalogue::TransportRouter route_manager;
};

void Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue,
               const renderer::MapRenderer& map_renderer,
               const transport_catalogue::TransportRouter&, std::ostream &output);

std::optional<DeserializeResult> Deserialize(std::istream& input);

namespace details {

TransportCatalogue Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue);
transport_catalogue::TransportCatalogue Deserialize(const TransportCatalogue& object);

MapRenderer Serialize(const renderer::MapRenderer& map_renderer);
renderer::MapRenderer Deserialize(const MapRenderer& object);

TransportRouter Serialize(const transport_catalogue::TransportRouter& transport_router);
transport_catalogue::TransportRouter Deserialize(const TransportRouter& object);

Stop Serialize(const transport_catalogue::Stop& stop);
Bus Serialize(const transport_catalogue::Bus& bus);

RenderSettings Serialize(const renderer::RenderSettings& render_settings);
renderer::RenderSettings Deserialize(const RenderSettings& object);

RoutingSettings Serialize(const transport_catalogue::RoutingSettings& routing_settings);
transport_catalogue::RoutingSettings Deserialize(const RoutingSettings& object);

Point Serialize(const svg::Point& point);
svg::Point Deserialize(const Point& object);

Color Serialize(const svg::Color& color);
svg::Color Deserialize(const Color& object);

Rgba Serialize(const svg::Rgba& rgb);
svg::Rgba Deserialize(const Rgba& object);

Rgb Serialize(const svg::Rgb& rgb);
svg::Rgb Deserialize(const Rgb& object);

} // namespace details

} // namespace transport_catalogue_serialize
