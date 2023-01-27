#include "json.h"
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

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

void MakeBase(const json::Document& document) {
    TransportCatalogue transport_catalogue;
    ParseBaseRequests(transport_catalogue, document);

    MapRenderer map_renderer(ParseRenderSettings(document));

    TransportRouter transport_router(ParseRoutingSettings(document), transport_catalogue);

    const auto& serialization_settings = ParseSerializationSettings(document);
    ofstream ofs(serialization_settings.file, ios::binary);
    transport_catalogue_serialize::Serialize(transport_catalogue, map_renderer, transport_router, ofs);
}

void ProcessRequests(const json::Document& document) {
    const auto& serialization_settings = ParseSerializationSettings(document);
    ifstream ifs(serialization_settings.file, ios::binary);

    if (auto result = transport_catalogue_serialize::Deserialize(ifs)) {
        auto& [transport_catalogue, map_renderer, transport_router] = *result;

        RequestHandler request_handler(transport_catalogue, map_renderer, transport_router);
        ParseStatRequests(request_handler, document, cout);

        // request_handler.RenderMap().Render(cout);
    }
}

int main(int argc, char* argv[]) {
    // TestAll();

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const auto& document = json::Load(cin);
    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        MakeBase(document);
    } else if (mode == "process_requests"sv) {
        ProcessRequests(document);
    } else {
        PrintUsage();
        return 1;
    }
}
