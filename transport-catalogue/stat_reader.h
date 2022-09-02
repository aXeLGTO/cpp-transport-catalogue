#pragma once

#include "request_handler.h"
#include "transport_catalogue.h"

#include <iostream>
#include <string>

namespace transport_catalogue::output {

void ReadQueries(const RequestHandler& request_handler, std::istream& input, std::ostream& output);

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

void PrintStop(const RequestHandler& request_handler, const Query& query, std::ostream& output);

void PrintBus(const RequestHandler& request_handler, const Query& query, std::ostream& output);

} // namespace detail

} // namespace transport_catalogue::output
