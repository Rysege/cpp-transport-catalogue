#include "json_reader.h"

#include <iostream>

using namespace std;

int main() {
    json_reader::JsonReader reader(std::cin);

    TransportCatalogue catalogue;
    handler::RequestHandler handler(catalogue);
    
    reader.LoadData(handler);
    reader.PrintStatRequest(handler, std::cout);
}