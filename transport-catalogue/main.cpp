#include "json_reader.h"

#include <iostream>

using namespace std;

int main() {
    json_reader::JsonReader reader(std::cin);

    TransportCatalogue catalogue;
    renderer::MapRenderer map_renderer;
    routemap::TransportRouter router;
    handler::RequestHandler handler(catalogue, router, map_renderer);

    reader.LoadData(handler);
    reader.PrintStatRequest(handler, std::cout);
}