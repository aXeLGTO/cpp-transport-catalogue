#pragma once
#include "domain.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "geo.h"

#include <iostream>
#include <string>

using namespace std::literals;

const double TOLERANCE = 1e-6;

bool operator==(const transport_catalogue::Stop& lhs, const transport_catalogue::Stop& rhs);
bool operator!=(const transport_catalogue::Stop& lhs, const transport_catalogue::Stop& rhs);

bool operator==(const transport_catalogue::Bus& lhs, const transport_catalogue::Bus& rhs);
bool operator!=(const transport_catalogue::Bus& lhs, const transport_catalogue::Bus& rhs);

bool operator==(const transport_catalogue::BusStat& lhs, const transport_catalogue::BusStat& rhs);
bool operator!=(const transport_catalogue::BusStat& lhs, const transport_catalogue::BusStat& rhs);

bool operator==(const svg::Point& lhs, const svg::Point& rhs);
bool operator!=(const svg::Point& lhs, const svg::Point& rhs);

bool operator==(const renderer::RenderSettings& lhs, const renderer::RenderSettings& rhs);
bool operator!=(const renderer::RenderSettings& lhs, const renderer::RenderSettings& rhs);

std::ostream& operator<<(std::ostream& out, const transport_catalogue::Stop& stop);

std::ostream& operator<<(std::ostream& out, const transport_catalogue::Bus& bus);

std::ostream& operator<<(std::ostream& out, const transport_catalogue::BusStat& bus_info);

template<typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& p) {
    using namespace std;
    return os << p.first << ": "s << p.second;
}

template<typename Container>
std::ostream& Print(std::ostream& os, const Container& container) {
    using namespace std;
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            os << ", "s;
        }

        is_first = false;
        os << element;
    }

    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::deque<T>& s) {
    os << '{';
    Print(os, s);
    return os << '}';
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& s) {
    os << '{';
    Print(os, s);
    return os << '}';
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << '{';
    Print(os, s);
    return os << '}';
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << '[';
    Print(os, v);
    return os << ']';
}

template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V>& s) {
    os << '{';
    Print(os, s);
    return os << '}';
}

void TestAddStop();
void TestAddBus();
void TestGetStopInfo();
void TestGetBusInfo();
void TestAddStopsDistance();

void TestParsePoint();
void TestParseColor();
void TestParseRenderSettings();

void TestAll();
