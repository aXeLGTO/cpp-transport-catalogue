#include "map_renderer.h"
#include "domain.h"
#include "svg.h"

#include <memory>

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace renderer {

using namespace std;
using namespace svg;
using namespace transport_catalogue;

MapRenderer::MapRenderer(RenderSettings settings) :
    settings_(move(settings)) {
}

const RenderSettings& MapRenderer::GetSetings() const {
    return settings_;
}

Polyline MapRenderer::RenderRouteLine(BusPtr bus, const Color& color, const SphereProjector& projector) const {
    const auto stops = MakeRoute(bus);
    Polyline route;

    for (const auto* stop : stops) {
        route.AddPoint(projector(stop->coordinates));
    }

    return route
        .SetFillColor(NoneColor)
        .SetStrokeColor(color)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND)
        .SetStrokeWidth(settings_.line_width);
}

void MapRenderer::RenderRouteName(const Point& position, const Color& color, const string& name, vector<Text>& out_texts) const {
    out_texts.push_back(Text()
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND)
        .SetPosition(position)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetData(name)
    );

    out_texts.push_back(Text()
        .SetFillColor(color)
        .SetPosition(position)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetData(name)
    );
}

void MapRenderer::RenderStopName(const Point& position, const string& name, Document& document) const {
    document.Add(Text()
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND)
        .SetPosition(position)
        .SetOffset(settings_.stop_label_offset)
        .SetFontSize(settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetData(name)
    );

    document.Add(Text()
        .SetFillColor("black"s)
        .SetPosition(position)
        .SetOffset(settings_.stop_label_offset)
        .SetFontSize(settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetData(name)
    );
}

} // namespace renderer
