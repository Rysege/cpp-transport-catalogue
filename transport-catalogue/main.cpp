#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"

using namespace catalog;

int main() {
    TransportCatalogue catalogue;

    input::LoadCatalogue(catalogue, std::cin);
    output::RequestCatalogue(catalogue, std::cin, std::cout);
}