#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <string>

namespace transport_catalogue::output {

void ReadQueries(const TransportCatalogue& catalogue, std::istream& input, std::ostream& output);

namespace detail {

enum class QueryType {
    PRINT_STOP,
    PRINT_BUS
};

struct Query {
    QueryType type;
    std::string name;

    auto MakeTuple() const {
        return tie(type, name);
    }

    bool operator==(const Query& other) const {
        return MakeTuple() == other.MakeTuple();
    }

    bool operator!=(const Query& other) const {
        return !(*this == other);
    }
};

std::ostream& operator<<(std::ostream& out, const Query& query);

Query ParseQuery(std::string_view raw_query);

void PrintStop(const TransportCatalogue& catalogue, const Query& query, std::ostream& output);

void PrintBus(const TransportCatalogue& catalogue, const Query& query, std::ostream& output);

} // namespace detail

} // namespace transport_catalogue::output
