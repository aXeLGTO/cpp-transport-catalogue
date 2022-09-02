#include "stat_reader.h"
#include "request_handler.h"

#include <iomanip>
#include <string_view>
#include <set>

using namespace std;

namespace transport_catalogue::output {

void ReadQueries(const RequestHandler& request_handler, istream& input, ostream& output) {
    using namespace detail;

    string line;
    int num_queries;
    input >> num_queries;
    getline(input, line);

    while(num_queries --> 0) {
        getline(input, line);
        auto query = ParseQuery(line);
        switch(query.type) {
            case QueryType::PRINT_STOP:
                PrintStop(request_handler, query, output);
                break;
            case QueryType::PRINT_BUS:
                PrintBus(request_handler, query, output);
                break;
        }

        output << endl;
    }
}

namespace detail {

Query ParseQuery(string_view raw_query) {
    static const string STOP_TOKEN = "Stop "s;
    static const string BUS_TOKEN = "Bus "s;

    size_t pos = raw_query.find(STOP_TOKEN);
    if (pos != string_view::npos) {
        return {QueryType::PRINT_STOP, string(raw_query.substr(pos + STOP_TOKEN.size()))};
    } else {
        size_t pos = raw_query.find(BUS_TOKEN);
        return {QueryType::PRINT_BUS, string(raw_query.substr(pos + BUS_TOKEN.size()))};
    }
}

ostream& operator<<(ostream& out, const Query& query) {
    return out << "Bus "s << query.name;
}

void PrintStop(const RequestHandler& request_handler, const Query& query, ostream& output) {
    output <<  "Stop "s  << query.name << ": "s;

    const auto* buses = request_handler.GetBusesByStop(query.name);
    if (buses) {
        if (buses->size() > 0) {
            set<string_view> bus_names;
            for (const auto* bus : *buses) {
                bus_names.insert(bus->name);
            }

            output << "buses ";
            bool isFirst = true;
            for (const auto name : bus_names) {
                if (!isFirst) {
                    output << " ";
                }
                isFirst = false;
                output << name;
            }
        } else {
            output << "no buses";
        }
    } else {
        output << "not found"s;
    }
}

void PrintBus(const RequestHandler& request_handler, const Query& query, ostream& output) {
    output <<  "Bus "s  << query.name << ": "s;

    if (auto bus_info = request_handler.GetBusStat(query.name)) {
        output
            << bus_info->stops_amount << " stops on route, "s
            << bus_info->unique_stops_amount << " unique stops, "s
            << setprecision(6)
            << bus_info->route_length << " route length, "s
            << setprecision(6)
            << bus_info->curvature << " curvature"s;
    } else {
        output << "not found"s;
    }
}

} // namespace detail

} // namespace transport_catalogue::output
