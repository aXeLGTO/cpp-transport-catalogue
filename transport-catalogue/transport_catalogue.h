#pragma once

#include "domain.h"
#include "ranges.h"
#include <optional>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <map>
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
    using BusIndexMap = std::map<std::string_view, BusPtr>;
    using StopBusesIndexMap = std::unordered_map<StopPtr, std::unordered_set<BusPtr>>;
    using StopsPair = std::pair<StopPtr, StopPtr>;

    auto begin() const {
        return bus_by_name_.begin();
    }

    auto end() const {
        return bus_by_name_.end();
    }

    void AddStop(const Stop& stop);

    const Stop& FindStop(std::string_view name) const;

    void AddBus(const Bus& bus);

    BusPtr FindBus(std::string_view name) const;

    std::optional<BusStat> GetBusStat(std::string_view bus_name) const;

    const std::unordered_set<BusPtr>* GetBusesByStop(std::string_view name) const;

    void SetDistance(const Stop& from, const Stop& to, double distance);

    double GetDistance(const Stop& from, const Stop& to) const;

    size_t GetBusesCount() const;

    size_t GetStopsCount() const;

    ranges::Range<std::deque<Stop>::const_iterator> GetStopsRange() const;

    ranges::Range<std::deque<Bus>::const_iterator> GetBusesRange() const;

private:
    std::deque<Stop> stops_;
    StopIndexMap stop_by_name_;

    std::deque<Bus> buses_;
    BusIndexMap bus_by_name_;

    StopBusesIndexMap stop_to_buses_;
    std::unordered_map<StopsPair, double, StopsPairHasher> stops_to_distance_;
};

} // namespace transport_catalogue
