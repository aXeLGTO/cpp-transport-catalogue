#pragma once
#include "transport_catalogue.h"

#include <iostream>

namespace transport_catalogue_serialize {

void SerializeTransportCatalogue(const transport_catalogue::TransportCatalogue& catalogue, std::ostream& output);

transport_catalogue::TransportCatalogue DeserializeTransportCatalogue(std::istream& input);

} // namespace transport_catalogue_serialize
