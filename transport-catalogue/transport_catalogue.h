#pragma once

#include "geo.h"
#include <optional>
#include <string>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <set>
#include <tuple>
#include <iostream>
#include <vector>

namespace transport_catalogue {

struct Stop {
    std::string name;
    Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
};

struct StopInfo {
    std::set<std::string_view> buses;
};

struct BusInfo {
    size_t stops_amount;
    size_t unique_stops_amount;
    double route_length;
    double curvature;
};

struct StopsPairHasher {
    std::size_t operator()(const std::pair<const Stop*, const Stop*>& stops_pair) const {
        return ptr_hasher(stops_pair.first) * 37 + ptr_hasher(stops_pair.second);
    }

private:
    std::hash<const void*> ptr_hasher;
};

class TransportCatalogue {
public:
    using StopIndexMap = std::unordered_map<std::string_view, const Stop*>;
    using BusIndexMap = std::unordered_map<std::string_view, const Bus*>;
    using StopBusesIndexMap = std::unordered_map<const Stop*, std::set<std::string_view>>;
    using StopsPair = std::pair<const Stop*, const Stop*>;

    void AddStop(const Stop& stop);

    const Stop& FindStop(std::string_view name) const;

    void AddBus(const Bus& bus);

    const Bus& FindBus(std::string_view name) const;

    std::optional<StopInfo> GetStopInfo(std::string_view name) const;

    std::optional<BusInfo> GetBusInfo(std::string_view name) const;

    void SetDistance(const Stop& from, const Stop& to, double distance);

    double GetDistance(const Stop& from, const Stop& to) const;

private:
    std::deque<Stop> stops;
    StopIndexMap stop_by_name;

    std::deque<Bus> buses;
    BusIndexMap bus_by_name;

    StopBusesIndexMap stop_to_busnames;
    std::unordered_map<StopsPair, double, StopsPairHasher> stops_to_distance;
};

} // namespace transport_catalogue
