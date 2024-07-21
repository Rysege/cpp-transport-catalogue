#pragma once
#include <iosfwd>

#include "transport_catalogue.h"

namespace catalog {
namespace output {

struct Query {
    std::string_view type;
    std::string_view text;
};

void RequestCatalogue(const TransportCatalogue& catalogue, std::istream& in, std::ostream& out);

void ProcessRequest(const TransportCatalogue& catalogue, std::string_view request, std::ostream& out);
} // namespase output
} // namespace catalog