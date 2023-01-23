#include "json_reader.h"
#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "router.h"
#include "serialization.h"

#include <fstream>
#include <iostream>
#include <string_view>

// #include "tests.h"

using namespace std;
using namespace transport_catalogue;
using namespace renderer;
using namespace transport_catalogue_serialize;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    // TestAll();

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const auto& document = json::Load(cin);
    const auto& serialization_settings = ParseSerializationSettings(document);

    const std::string_view mode(argv[1]);
    if (mode == "make_base"sv) {
        TransportCatalogue catalogue;
        ParseBaseRequests(catalogue, document);

        ofstream ofs(serialization_settings.file, ios::binary);
        SerializeTransportCatalogue(catalogue, ofs);
    } else if (mode == "process_requests"sv) {
        ifstream ifs(serialization_settings.file, ios::binary);
        TransportCatalogue catalogue = DeserializeTransportCatalogue(ifs);

        MapRenderer renderer(ParseRenderSettings(document));

        TransportRouter route_manager(ParseRoutingSettings(document), catalogue);

        RequestHandler request_handler(catalogue, renderer, route_manager);
        ParseStatRequests(request_handler, document, cout);

        // request_handler.RenderMap().Render(cout);
    } else {
        PrintUsage();
        return 1;
    }
}
