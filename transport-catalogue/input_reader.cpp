#include "input_reader.h"
#include <deque>
#include <string>

using namespace std;

namespace transport_catalogue::input {

void ReadQueries(TransportCatalogue& catalogue, std::istream& input) {
    using namespace detail;

    string line;
    int num_queries;
    input >> num_queries;
    getline(input, line);
    deque<string> lines;

    vector<AddStopQuery> stop_queries;
    vector<AddBusQuery> bus_queries;
    while(num_queries --> 0) {
        getline(input, line);
        lines.push_back(move(line));
        auto [type, rest_query] = ParseQueryType(lines.back());
        switch (type) {
            case QueryType::ADD_STOP: {
                auto query = ParseAddStopQuery(rest_query);
                stop_queries.push_back(query);
                catalogue.AddStop(ParseStop(query));
                break;
            }
            case QueryType::ADD_BUS: {
                bus_queries.push_back(ParseAddBusQuery(rest_query));
                break;
            }
        }
    }

    for (const auto& query : stop_queries) {
        auto* stop = &catalogue.FindStop(query.name);
        for (const auto& [name, distance] : query.distances) {
            catalogue.SetDistance(*stop, catalogue.FindStop(name), distance);
        }
    }

    for (const auto& query : bus_queries) {
        vector<const Stop*> stops;
        for (string_view name : query.stops) {
            stops.push_back(&catalogue.FindStop(name));
        }
        catalogue.AddBus(Bus{string(query.name), move(stops)});
    }
}

namespace detail {

Stop ParseStop(const AddStopQuery& query) {
    return {string(query.name), {query.latitude, query.longitude}};
}

void ParseBusRoute(string_view data, vector<string_view>& stops) {
    if (data.find('-') == string_view::npos) {
        stops.push_back(Trim(data));
    } else {
        auto [left, right] = Split(data, '-');
        stops.push_back(Trim(left));
        ParseBusRoute(right, stops);
        stops.push_back(Trim(left));
    }
}

void ParseBusCircularRoute(string_view data, vector<string_view>& stops) {
    if (data.find('>') == string_view::npos) {
        stops.push_back(Trim(data));
    } else {
        auto [left, right] = Split(data, '>');
        stops.push_back(Trim(left));
        ParseBusCircularRoute(right, stops);
    }
}

AddStopQuery ParseAddStopQuery(std::string_view raw_query) {
    size_t start_pos = 0;
    size_t end_pos = raw_query.find_first_of(':');
    auto name = raw_query.substr(start_pos, end_pos);

    start_pos = end_pos + 2;
    end_pos = raw_query.find_first_of(',', start_pos);
    double latitude = stod(string(raw_query.substr(start_pos, end_pos - start_pos)));

    start_pos = end_pos + 2;
    end_pos = raw_query.find_first_of(',', start_pos);
    double longitude = stod(string(raw_query.substr(start_pos, end_pos - start_pos)));

    vector<pair<string_view, double>> distances;
    while(end_pos != string_view::npos) {
        start_pos = end_pos + 2;
        end_pos = raw_query.find_first_of(' ', start_pos);
        double distance = stod(string(raw_query.substr(start_pos, end_pos - 1 - start_pos)));

        start_pos = end_pos + 4;
        end_pos = raw_query.find_first_of(',', start_pos);
        auto name = raw_query.substr(start_pos, end_pos - start_pos);
        distances.push_back({name, distance});
    }

    return {name, latitude, longitude, distances};
}

AddBusQuery ParseAddBusQuery(string_view raw_query) {
    size_t start_pos = 0;
    size_t end_pos = raw_query.find_first_of(':');
    auto name = raw_query.substr(start_pos, end_pos);

    vector<string_view> stops;
    if (raw_query.find('>') != string_view::npos) {
        ParseBusCircularRoute(raw_query.substr(end_pos + 2), stops);
    } else {
        ParseBusRoute(raw_query.substr(end_pos + 2), stops);
    }

    return {name, move(stops)};
}

pair<string_view, string_view> Split(string_view line, char by) {
    size_t pos = line.find(by);
    string_view left = line.substr(0, pos);

    if (pos < line.size() && pos + 1 < line.size()) {
        return {left, line.substr(pos + 1)};
    } else {
        return {left, string_view()};
    }
}

std::tuple<QueryType, std::string_view> ParseQueryType(std::string_view raw_query) {
    size_t pos = raw_query.find_first_of(' ');
    auto type_str = raw_query.substr(0, pos);
    QueryType type;
    if (type_str == "Stop"s) {
        type = QueryType::ADD_STOP;
    } else if (type_str == "Bus"s) {
        type = QueryType::ADD_BUS;
    }
    return make_tuple(type, raw_query.substr(pos + 1));
}

string_view Lstrip(string_view line) {
    while (!line.empty() && isspace(line[0])) {
        line.remove_prefix(1);
    }
    return line;
}

string_view Rstrip(string_view str) {
    while (!str.empty() && isspace(str[str.size() - 1])) {
        str.remove_suffix(1);
    }
    return str;
}

string_view Trim(string_view str) {
    return Rstrip(Lstrip(str));
}

} // namespace detail

} //namespace transport_catalogue::input
