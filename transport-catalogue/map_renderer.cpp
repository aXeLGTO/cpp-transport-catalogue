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

Polyline MapRenderer::RenderRouteLine(BusPtr bus, const Color& color, const SphereProjector& projector, Document& document) const {
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
        .SetPosition(position)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(StrokeLineCap::ROUND)
        .SetStrokeLineJoin(StrokeLineJoin::ROUND)
        .SetData(name)
    );

    out_texts.push_back(Text()
        .SetPosition(position)
        .SetOffset(settings_.bus_label_offset)
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetFillColor(color)
        .SetData(name)
    );
}

} // namespace renderer
