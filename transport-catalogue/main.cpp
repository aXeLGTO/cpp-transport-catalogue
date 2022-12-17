#include "json_reader.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "router.h"
// #include "tests.h"

using namespace std;
using namespace transport_catalogue;
using namespace renderer;

int main() {
    // TestAll();

    const auto& document = json::Load(cin);

    TransportCatalogue catalogue;
    ParseBaseRequests(catalogue, document);

    MapRenderer renderer(ParseRenderSettings(document));

    TransportRouter route_manager(ParseRoutingSettings(document), catalogue);

    RequestHandler request_handler(catalogue, renderer, route_manager);
    ParseStatRequests(request_handler, document, cout);

    // request_handler.RenderMap().Render(cout);

    return 0;
}
