#include "map_renderer.h"

#include <memory>

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace renderer {

using namespace std;

MapRenderer::MapRenderer(RenderSettings settings) :
    settings_(move(settings)) {
}

const RenderSettings& MapRenderer::GetSetings() const {
    return settings_;
}

} // namespace renderer
