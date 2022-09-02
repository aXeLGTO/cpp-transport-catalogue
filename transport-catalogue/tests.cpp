#include "tests.h"
#include "input_reader.h"
#include "request_handler.h"
#include "test_framework.h"

#include <sstream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;
using namespace transport_catalogue;

bool operator==(const Stop& lhs, const Stop& rhs) {
    if (lhs.name == rhs.name) {
        return abs(lhs.coordinates.lat - rhs.coordinates.lat) < TOLERANCE
            && abs(lhs.coordinates.lng - rhs.coordinates.lng) < TOLERANCE;
    }
    return false;
}

bool operator!=(const Stop& lhs, const Stop& rhs) {
    return !(lhs == rhs);
}

bool operator==(const Bus& lhs, const Bus& rhs) {
    return tie(lhs.name, lhs.stops) == tie(rhs.name, rhs.stops);
}

bool operator!=(const Bus& lhs, const Bus& rhs) {
    return !(lhs == rhs);
}

bool operator==(const BusStat& lhs, const BusStat& rhs) {
    if (tie(lhs.stops_amount, lhs.unique_stops_amount) ==
        tie(rhs.stops_amount, rhs.unique_stops_amount)) {
        return abs(lhs.route_length - rhs.route_length) < TOLERANCE;
    }

    return false;
}

bool operator!=(const BusStat& lhs, const BusStat& rhs) {
    return !(lhs == rhs);
}

ostream& operator<<(ostream& out, const Stop& stop) {
    return out << "("s << stop.name << ", "s << stop.coordinates.lat << ", "s << stop.coordinates.lng << ")"s;
}

ostream& operator<<(ostream& out, const Bus& bus) {
    return out << "("s << bus.name << ", "s << bus.stops << ")"s;
}

ostream& operator<<(ostream& out, const BusStat& bus_info) {
    return out << "("s
        << bus_info.stops_amount << ", "s
        << bus_info.unique_stops_amount << ", "s
        << bus_info.route_length << ", "s
        << bus_info.curvature << ")"s;
}

ostream& operator<<(ostream& out, const transport_catalogue::input::detail::AddStopQuery& query) {
    return out << "("s
        << query.name << ", "s
        << query.latitude << ", "s
        << query.longitude << ", "s
        << query.distances << ")"s;
}

ostream& operator<<(ostream& out, const transport_catalogue::input::detail::AddBusQuery& query) {
    return out << "("s
        << query.name << ", "s
        << query.stops << ")"s;
}

void TestAddStop() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        ASSERT_EQUAL(catalogue.FindStop(stop1.name), stop1);
    }
}

void TestAddBus() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);

        Bus bus1{"256"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name)
        }};
        catalogue.AddBus(bus1);
        ASSERT_EQUAL(*catalogue.FindBus("256"s), bus1);
    }
}

void TestGetStopInfo() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    {
        TransportCatalogue catalogue;
        const auto* empty = catalogue.GetBusesByStop("256"s);
        ASSERT(!empty);
    }
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        Bus bus1{"750"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name)
        }};
        Bus bus2{"256"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus1);
        catalogue.AddBus(bus2);

        unordered_map<string_view, set<string_view>> stop_to_busnames{
            {stop1.name, {bus1.name, bus2.name}},
            {stop2.name, {bus1.name, bus2.name}},
            {stop3.name, {bus2.name}}
        };
        for (const auto& [stop_name, busnames] : stop_to_busnames) {
            const auto* busesPtr = catalogue.GetBusesByStop(stop_name);
            ASSERT(busesPtr);
            vector<BusPtr> buses(busesPtr->begin(), busesPtr->end());
            for (const auto& name : busnames) {
                ASSERT(find_if(buses.begin(), buses.end(), [&name](const auto* v){return v->name == name;}) != buses.end());
            }
        }
    }
}

void TestGetBusInfo() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    double distance_stop1_to_stop2 = 3900;
    double distance_stop2_to_stop3 = 9900;
    double distance_stop2_to_stop2 = 100;
    double distance_stop3_to_stop2 = 9500;
    using namespace geo;
    auto coord_distance = ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng})
        + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
        + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
        + ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng});
    auto distance = distance_stop1_to_stop2 + distance_stop2_to_stop2 + distance_stop2_to_stop3 + distance_stop3_to_stop2 + distance_stop2_to_stop2 + distance_stop1_to_stop2;

    {
        RequestHandler request_handler{TransportCatalogue{}};
        const auto empty = request_handler.GetBusStat("256"s);
        ASSERT(!empty);
    }

    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop1.name),
            catalogue.FindStop(stop2.name),
        distance_stop1_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop3.name),
        distance_stop2_to_stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop2.name),
        distance_stop2_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop3.name),
            catalogue.FindStop(stop2.name),
        distance_stop3_to_stop2);

        Bus bus{"750"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus);

        RequestHandler request_handler{catalogue};
        auto bus_info = request_handler.GetBusStat(bus.name);
        ASSERT(bus_info);
        ASSERT_EQUAL(bus_info->stops_amount, 7u);
        ASSERT_EQUAL(bus_info->unique_stops_amount, 3u);
        ASSERT(abs(bus_info->route_length - distance) < TOLERANCE);
        ASSERT(abs(bus_info->curvature - (distance / coord_distance)) < TOLERANCE);
    }
}

void TestAddStopsDistance() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    double distance_stop1_to_stop2 = 3900;
    double distance_stop2_to_stop3 = 9900;
    double distance_stop2_to_stop2 = 100;
    double distance_stop3_to_stop2 = 9500;
    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop1.name),
            catalogue.FindStop(stop2.name),
            distance_stop1_to_stop2
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop3.name),
            distance_stop2_to_stop3
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop2.name),
            distance_stop2_to_stop2
        );

        catalogue.SetDistance(
            catalogue.FindStop(stop3.name),
            catalogue.FindStop(stop2.name),
            distance_stop3_to_stop2
        );

        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop1.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop1_to_stop2);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop2.name),
                    catalogue.FindStop(stop3.name)),
                distance_stop2_to_stop3);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop2.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop2_to_stop2);
        ASSERT_EQUAL(catalogue.GetDistance(
                    catalogue.FindStop(stop3.name),
                    catalogue.FindStop(stop2.name)),
                distance_stop3_to_stop2);

        ASSERT_EQUAL(catalogue.GetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop1.name)
        ), distance_stop1_to_stop2);
    }
}

void TestOutputParseQuery() {
    using namespace output::detail;
    Query query = {QueryType::PRINT_BUS, "750"s};
    ASSERT_EQUAL(ParseQuery("Bus 750"s), query);
}

void TestStatReader() {
    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};
    double distance_stop1_to_stop2 = 3900;
    double distance_stop2_to_stop3 = 9900;
    double distance_stop2_to_stop2 = 100;
    double distance_stop3_to_stop2 = 9500;

    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop1.name),
            catalogue.FindStop(stop2.name),
        distance_stop1_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop3.name),
        distance_stop2_to_stop3);

        catalogue.SetDistance(
            catalogue.FindStop(stop2.name),
            catalogue.FindStop(stop2.name),
        distance_stop2_to_stop2);

        catalogue.SetDistance(
            catalogue.FindStop(stop3.name),
            catalogue.FindStop(stop2.name),
        distance_stop3_to_stop2);

        Bus bus{"750"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus);

        RequestHandler request_handler{catalogue};
        auto bus_info = request_handler.GetBusStat(bus.name);
        ASSERT(bus_info);

        ostringstream expected_output;
        expected_output << "Bus "s
            << bus.name << ": "s
            << bus_info->stops_amount << " stops on route, "s
            << bus_info->unique_stops_amount << " unique stops, "s
            << setprecision(6)
            << bus_info->route_length << " route length, "s
            << setprecision(6)
            << bus_info->curvature << " curvature\n"s;

        istringstream input{
            "1\n"
            "Bus 750\n"
        };
        ostringstream output;
        output::ReadQueries(RequestHandler{catalogue}, input, output);

        ASSERT_EQUAL(output.str(), expected_output.str());
    }

    {
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        catalogue.AddStop(stop2);
        catalogue.AddStop(stop3);

        Bus bus1{"256"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus1);
        Bus bus2{"750"s, {
            &catalogue.FindStop(stop1.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop3.name),
            &catalogue.FindStop(stop2.name),
            &catalogue.FindStop(stop1.name),
        }};
        catalogue.AddBus(bus2);

        ostringstream expected_output;
        expected_output << "Stop "s
            << stop2.name << ": buses "s
            << bus1.name << " "s
            << bus2.name << "\n"s;

        istringstream input{
            "1\n"
            "Stop Marushkino\n"
        };
        ostringstream output;
        output::ReadQueries(RequestHandler{catalogue}, input, output);

        ASSERT_EQUAL(output.str(), expected_output.str());
    }
    {
        istringstream input{
            "1\n"
            "Stop Samara\n"
        };
        ostringstream output;
        output::ReadQueries(RequestHandler{TransportCatalogue()}, input, output);

        ASSERT_EQUAL(output.str(), "Stop Samara: not found\n"s);
    }
    {
        istringstream input{
            "1\n"
            "Stop Tolstopaltsevo\n"
        };
        ostringstream output;
        TransportCatalogue catalogue;
        catalogue.AddStop(stop1);
        output::ReadQueries(RequestHandler{catalogue}, input, output);

        ASSERT_EQUAL(output.str(), "Stop Tolstopaltsevo: no buses\n"s);
    }
    {
        istringstream input{
            "1\n"
            "Bus 220\n"
        };
        ostringstream output;
        output::ReadQueries(RequestHandler{TransportCatalogue()}, input, output);

        ASSERT_EQUAL(output.str(), "Bus 220: not found\n"s);
    }
}

void TestInputParseQuery() {
    using namespace input::detail;
    {
        string raw_query = "Stop  Biryulyovo  Zapadnoye : 55.574371, 37.651700, 9900m to Rasskazovka, 100m to Marushkino"s;
        AddStopQuery query = {" Biryulyovo  Zapadnoye "sv, 55.574371, 37.651700, {{"Rasskazovka"sv, 9900}, {"Marushkino"sv, 100}}};
        auto [_, rest_query] = ParseQueryType(raw_query);
        ASSERT_EQUAL(ParseAddStopQuery(rest_query), query);
    }
    {
        string raw_query = "Bus  A  750 : Tolstopaltsevo - Marushkino - Rasskazovka"s;
        AddBusQuery query = {" A  750 "sv, {"Tolstopaltsevo"sv, "Marushkino"sv, "Rasskazovka"sv, "Marushkino"sv, "Tolstopaltsevo"sv}};
        auto [_, rest_query] = ParseQueryType(raw_query);
        ASSERT_EQUAL(ParseAddBusQuery(rest_query), query);
    }
    {
        string raw_query = "Bus 750: Tolstopaltsevo > Marushkino > Rasskazovka > Tolstopaltsevo"s;
        AddBusQuery query = {"750"sv, {"Tolstopaltsevo"sv, "Marushkino"sv, "Rasskazovka"sv, "Tolstopaltsevo"sv}};
        auto [_, rest_query] = ParseQueryType(raw_query);
        ASSERT_EQUAL(ParseAddBusQuery(rest_query), query);
    }
}

void TestInputReader() {
    using namespace input;

    Stop stop1{"Tolstopaltsevo"s, {55.611087, 37.208290}};
    Stop stop2{"Marushkino"s, {55.595884, 37.209755}};
    Stop stop3{"Rasskazovka"s, {55.632761, 37.333324}};

    {
        Bus exp_bus{"750"s, vector<StopPtr>{&stop1, &stop2, &stop3, &stop2, &stop1}};

        using namespace geo;
        double coord_distance = ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng})
            + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
            + ComputeDistance({stop3.coordinates.lat, stop3.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng})
            + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop1.coordinates.lat, stop1.coordinates.lng});
        double distance = 3900 + 9900 + 9500 + 3900;
        BusStat exp_bus_info{5, 3, distance, distance / coord_distance};

        TransportCatalogue catalogue;
        istringstream input{
            "4\n"
            "Stop Tolstopaltsevo: 55.611087, 37.208290, 3900m to Marushkino\n"
            "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
            "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n"
            "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Marushkino\n"
        };
        ReadQueries(catalogue, input);

        ASSERT_EQUAL(catalogue.FindStop("Marushkino"s), stop2);

        const auto* bus = catalogue.FindBus("750"s);
        ASSERT_EQUAL(bus->name, exp_bus.name);
        ASSERT_EQUAL(bus->stops.size(), exp_bus.stops.size());
        for (size_t i = 0; i < exp_bus.stops.size(); ++i) {
            ASSERT_EQUAL(*bus->stops[i], *exp_bus.stops[i]);
        }
        RequestHandler request_handler{catalogue};
        auto bus_info = request_handler.GetBusStat("750"s);
        ASSERT(bus_info);
        ASSERT_EQUAL(*bus_info, exp_bus_info);
    }

    // circular route
    {
        Bus exp_bus{"256"s, vector<StopPtr>{&stop1, &stop2, &stop3, &stop1}};

        using namespace geo;
        double coord_distance = ComputeDistance({stop1.coordinates.lat, stop1.coordinates.lng}, {stop2.coordinates.lat, stop2.coordinates.lng})
            + ComputeDistance({stop2.coordinates.lat, stop2.coordinates.lng}, {stop3.coordinates.lat, stop3.coordinates.lng})
            + ComputeDistance({stop3.coordinates.lat, stop3.coordinates.lng}, {stop1.coordinates.lat, stop1.coordinates.lng});
        double distance = 3900 + 9900 + 9500;

        BusStat exp_bus_info{4, 3, distance, distance / coord_distance};

        TransportCatalogue catalogue;
        istringstream input{
            "4\n"
            "Stop Tolstopaltsevo: 55.611087, 37.208290, 3900m to Marushkino\n"
            "Bus 256: Tolstopaltsevo > Marushkino > Rasskazovka > Tolstopaltsevo\n"
            "Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino\n"
            "Stop Rasskazovka: 55.632761, 37.333324, 9500m to Tolstopaltsevo\n"
        };
        ReadQueries(catalogue, input);

        ASSERT_EQUAL(catalogue.FindStop("Marushkino"s), stop2);

        const auto* bus = catalogue.FindBus("256"s);
        ASSERT_EQUAL(bus->name, exp_bus.name);
        ASSERT_EQUAL(bus->stops.size(), exp_bus.stops.size());
        for (size_t i = 0; i < exp_bus.stops.size(); ++i) {
            ASSERT_EQUAL(*bus->stops[i], *exp_bus.stops[i]);
        }
        RequestHandler request_handler{catalogue};
        auto bus_info = request_handler.GetBusStat("256"s);
        ASSERT(bus_info);
        ASSERT_EQUAL(*bus_info, exp_bus_info);
    }
}

void TestAll() {
    RUN_TEST(TestAddStop);
    RUN_TEST(TestAddBus);
    RUN_TEST(TestGetBusInfo);
    RUN_TEST(TestGetStopInfo);
    RUN_TEST(TestAddStopsDistance);

    RUN_TEST(TestOutputParseQuery);
    RUN_TEST(TestStatReader);
    RUN_TEST(TestInputParseQuery);
    RUN_TEST(TestInputReader);

    cerr << "All tests done"s << endl;
}

