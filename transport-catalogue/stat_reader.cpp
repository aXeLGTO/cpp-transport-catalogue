#include "stat_reader.h"

#include <iomanip>

using namespace std;

namespace transport_catalogue::output {

void ReadQueries(const TransportCatalogue& catalogue, istream& input, ostream& output) {
    using namespace detail;

    string line;
    int num_queries;
    input >> num_queries;
    getline(input, line);

    vector<Query> queries;
    queries.reserve(num_queries);
    while(num_queries --> 0) {
        getline(input, line);
        queries.push_back(ParseQuery(line));
    }

    for (const auto& query : queries) {
        switch(query.type) {
            case QueryType::PRINT_STOP:
                PrintStop(catalogue, query, output);
                break;
            case QueryType::PRINT_BUS:
                PrintBus(catalogue, query, output);
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

void PrintStop(const TransportCatalogue& catalogue, const Query& query, ostream& output) {
    output <<  "Stop "s  << query.name << ": "s;

    auto [sucess, stop_info] = catalogue.GetStopInfo(query.name);
    if (sucess) {
        if (stop_info.buses.size() > 0) {
            output << "buses ";
            bool isFirst = true;
            for (const auto bus : stop_info.buses) {
                if (!isFirst) {
                    output << " ";
                }
                isFirst = false;
                output << bus;
            }
        } else {
            output << "no buses";
        }
    } else {
        output << "not found"s;
    }
}

void PrintBus(const TransportCatalogue& catalogue, const Query& query, ostream& output) {
    output <<  "Bus "s  << query.name << ": "s;

    auto [sucess, bus_info] = catalogue.GetBusInfo(query.name);
    if (sucess) {
        output
            << bus_info.stops_amount << " stops on route, "s
            << bus_info.unique_stops_amount << " unique stops, "s
            << setprecision(6)
            << bus_info.route_length << " route length, "s
            << setprecision(6)
            << bus_info.curvature << " curvature"s;
    } else {
        output << "not found"s;
    }
}

} // namespace detail

} // namespace transport_catalogue::output
