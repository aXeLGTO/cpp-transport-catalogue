#include "transport_catalogue.h"
#include "stat_reader.h"
#include "input_reader.h"
#include "tests.h"

using namespace std;
using namespace transport_catalogue;

int main() {
    TestAll();

    TransportCatalogue catalogue;

    input::ReadQueries(catalogue, cin);
    output::ReadQueries(catalogue, cin, cout);

    return 0;
}
