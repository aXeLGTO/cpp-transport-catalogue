#pragma once

#include "domain.h"
#include <optional>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace transport_catalogue {

struct StopsPairHasher {
    std::size_t operator()(const std::pair<StopPtr, StopPtr>& stops_pair) const {
        return ptr_hasher(stops_pair.first) * 37 + ptr_hasher(stops_pair.second);
    }

private:
    std::hash<const void*> ptr_hasher;
};

class TransportCatalogue {
public:
    using StopIndexMap = std::unordered_map<std::string_view, StopPtr>;
    using BusIndexMap = std::unordered_map<std::string_view, BusPtr>;
    using StopBusesIndexMap = std::unordered_map<StopPtr, std::unordered_set<BusPtr>>;
    using StopsPair = std::pair<StopPtr, StopPtr>;

    void AddStop(const Stop& stop);

    const Stop& FindStop(std::string_view name) const;

    void AddBus(const Bus& bus);

    BusPtr FindBus(std::string_view name) const;

    const std::unordered_set<BusPtr>* GetBusesByStop(std::string_view name) const;

    void SetDistance(const Stop& from, const Stop& to, double distance);

    double GetDistance(const Stop& from, const Stop& to) const;

private:
    std::deque<Stop> stops;
    StopIndexMap stop_by_name;

    std::deque<Bus> buses;
    BusIndexMap bus_by_name;

    StopBusesIndexMap stop_to_buses;
    std::unordered_map<StopsPair, double, StopsPairHasher> stops_to_distance;
};

} // namespace transport_catalogue
