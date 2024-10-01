#include "json_reader.h"

#include <iostream>

using namespace std;

int main() {
    catalog::TransportCatalogue catalogue;
    json_reader::JsonReader reader(catalogue, std::cin);
    reader.PrintStatRequest(std::cout);
}