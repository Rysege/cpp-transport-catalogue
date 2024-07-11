#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"

using namespace catalog;

int main() {
    TransportCatalogue catalogue;

    input::LoadCatalogue(std::cin, catalogue);
    output::RequestCatalogue(std::cin, std::cout, catalogue);
}