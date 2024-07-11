#pragma once
#include <iosfwd>

#include "transport_catalogue.h"

namespace catalog {
namespace output {

struct Query {
    std::string type;
    std::string text;
};

void ParseAndPrint(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
} // namespase output
} // namespace catalog