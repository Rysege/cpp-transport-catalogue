#pragma once
#include <iosfwd>

#include "transport_catalogue.h"

namespace catalog {
namespace output {

struct Query {
    std::string_view type;
    std::string_view text;
};

void ParseAndPrint(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output);
} // namespase output
} // namespace catalog