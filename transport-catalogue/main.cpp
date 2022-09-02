#include "json_reader.h"
#include "transport_catalogue.h"
#include "tests.h"

using namespace std;
using namespace transport_catalogue;

int main() {
    TestAll();

    TransportCatalogue catalogue;
    ReadRequests(catalogue, cin, cout);

    return 0;
}
