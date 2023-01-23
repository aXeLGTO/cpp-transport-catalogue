#pragma once
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "svg.h"

#include <iostream>
#include <transport_catalogue.pb.h>

#define SET_X(src, dst, field)    \
    dst.set_##field(src.field)

#define GET_X(src, dst, field)    \
    dst.field = src.field()

namespace transport_catalogue_serialize {

void SerializeTransportCatalogue(const transport_catalogue::TransportCatalogue &catalogue, const renderer::RenderSettings& settings, std::ostream &output);
bool DeserializeTransportCatalogue(std::istream& input, transport_catalogue::TransportCatalogue& catalogue, renderer::RenderSettings& settings);

namespace details {

RenderSettings SerializeRenderSetings(const renderer::RenderSettings& render_settings);
void DeserializeRenderSetings(const RenderSettings& render_settings_raw, renderer::RenderSettings& render_settings);

Point SerializePoint(const svg::Point& point);
svg::Point DeserializePoint(const Point& point_raw);

Color SerializeColor(const svg::Color& color);
svg::Color DeserializeColor(const Color& color_raw);

Rgba SerializeRgba(const svg::Rgba& rgb);
svg::Rgba DeserializeRgba(const Rgba& rgba);

Rgb SerializeRgb(const svg::Rgb& rgb);
svg::Rgb DeserializeRgb(const Rgb& rgb);

} // namespace details

} // namespace transport_catalogue_serialize
