#include "json_reader.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "tests.h"

using namespace std;
using namespace transport_catalogue;
using namespace renderer;

int main() {
    // TestAll();

    const auto& document = json::Load(cin);

    TransportCatalogue catalogue;
    ParseBaseRequests(catalogue, document);

    MapRenderer renderer(ParseRenderSettings(document));
    RequestHandler request_handler(catalogue, renderer);
    ParseStatRequests(request_handler, document, cout);

    return 0;
}
