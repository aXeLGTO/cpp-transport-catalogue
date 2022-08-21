#pragma once

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
    double latitude;
    double longitude;

    auto MakeTuple() const;

    bool operator==(const Stop& other) const;
    bool operator!=(const Stop& other) const;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;

    auto MakeTuple() const;

    bool operator==(const Bus& other) const;
    bool operator!=(const Bus& other) const;
};

struct StopInfo {
    std::set<std::string_view> buses;

    auto MakeTuple() const;

    bool operator==(const StopInfo& other) const;
    bool operator!=(const StopInfo& other) const;
};

struct BusInfo {
    size_t stops_amount;
    size_t unique_stops_amount;
    double route_length;
    double curvature;

    auto MakeTuple() const;

    bool operator==(const BusInfo& other) const;
    bool operator!=(const BusInfo& other) const;
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

    void AddStop(Stop stop);

    const Stop& FindStop(std::string_view name) const;

    void AddBus(Bus bus);

    const Bus& FindBus(std::string_view name) const;

    std::tuple<bool, StopInfo> GetStopInfo(std::string_view name) const;

    std::tuple<bool, BusInfo> GetBusInfo(std::string_view name) const;

    void AddDistance(StopsPair stops_pair, double distance);

    double GetDistance(StopsPair stops_pair) const;

private:
    std::deque<Stop> stops;
    StopIndexMap stopname_to_stop;

    std::deque<Bus> buses;
    BusIndexMap busname_to_bus;

    StopBusesIndexMap stop_to_busnames;
    std::unordered_map<StopsPair, double, StopsPairHasher> stops_to_distance;
};

} // namespace transport_catalogue
