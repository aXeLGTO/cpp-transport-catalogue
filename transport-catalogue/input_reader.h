#pragma once
#include "domain.h"
#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace transport_catalogue::input {

void ReadQueries(TransportCatalogue& catalogue, std::istream& input);

namespace detail {

enum class QueryType {
    ADD_STOP,
    ADD_BUS
};

struct AddStopQuery {
    std::string_view name;
    double latitude;
    double longitude;
    std::vector<std::pair<std::string_view, double>> distances;

    auto MakeTuple() const {
        return tie(name, latitude, longitude, distances);
    }

    bool operator==(const AddStopQuery& other) const {
        return MakeTuple() == other.MakeTuple();
    }

    bool operator!=(const AddStopQuery& other) const {
        return !(*this == other);
    }
};

struct AddBusQuery {
    std::string_view name;
    std::vector<std::string_view> stops;

    auto MakeTuple() const {
        return tie(name, stops);
    }

    bool operator==(const AddBusQuery& other) const {
        return MakeTuple() == other.MakeTuple();
    }

    bool operator!=(const AddBusQuery& other) const {
        return !(*this == other);
    }
};

AddStopQuery ParseAddStopQuery(std::string_view raw_query);

AddBusQuery ParseAddBusQuery(std::string_view raw_query);

Stop ParseStop(const AddStopQuery& query);

void ParseBusRoute(const TransportCatalogue& catalogue, std::string_view data, std::vector<const Stop*>& stops);

void ParseBusCircularRoute(const TransportCatalogue& catalogue, std::string_view data, std::vector<const Stop*>& stops);

std::tuple<QueryType, std::string_view> ParseQueryType(std::string_view raw_type);

std::pair<std::string_view, std::string_view> Split(std::string_view line, char by);

std::string_view Lstrip(std::string_view line);

std::string_view Rstrip(std::string_view line);

std::string_view Trim(std::string_view line);

} // namespace detail

} // namespace transport_catalogue::input
