#include <iostream>
#include <sstream>

#include "json_reader.h"

#include <fstream>

using namespace std;

void Test() {
    string s = R"({
    "base_requests": [
    {
        "type": "Bus",
        "name": "114",
        "stops": ["Морской вокзал", "Ривьерский мост"],
        "is_roundtrip": false
    },
    {
        "type": "Stop",
        "name": "Ривьерский мост",
        "latitude": 43.587795,
        "longitude": 39.716901,
        "road_distances": {"Морской вокзал": 850}
    },
    {
        "type": "Stop",
        "name": "Морской вокзал",
        "latitude": 43.581969,
        "longitude": 39.719848,
        "road_distances": {"Ривьерский мост": 850}
    }
    ],
    "render_settings": {
    "width": 200,
    "height": 200,
    "padding": 30,
    "stop_radius": 5,
    "line_width": 14,
    "bus_label_font_size": 20,
    "bus_label_offset": [7, 15],
    "stop_label_font_size": 20,
    "stop_label_offset": [7, -3],
    "underlayer_color": [255,255,255,0.85],
    "underlayer_width": 3,
    "color_palette": ["green", [255,160,0],"red"]
    },
    "stat_requests": [
    { "id": 1, "type": "Map" },
    { "id": 2, "type": "Stop", "name": "Ривьерский мост" },
    { "id": 3, "type": "Bus", "name": "114" }
    ]
} )";
    istringstream is(s);

    catalog::TransportCatalogue catalogue;
    try {
        json_reader::JsonReader reader(is);
        renderer::MapRenderer map_renderer;
        handler::RequestHandler handler(catalogue, map_renderer);
        reader.LoadDataToCatalogue(handler);
        reader.LoadRenderSetting(handler);
        reader.PrintStatRequest(handler, std::cout);
        
    }
    catch (const std::exception& err) {
        std::cerr << err.what();
    }
}

void Test2() {

    ifstream f;
    f.open("case_input_24.txt", ios::in);
    catalog::TransportCatalogue catalogue;
    try {
        json_reader::JsonReader reader(f);
        renderer::MapRenderer map_renderer;
        handler::RequestHandler handler(catalogue, map_renderer);
        reader.LoadDataToCatalogue(handler);
        reader.PrintStatRequest(handler, std::cout);
    }
    catch (const std::exception& err) {
        std::cerr << err.what();
    }
}

void TestRender() {

    ifstream f;
    f.open("case001.txt", ios::in);
    catalog::TransportCatalogue catalogue;
    try {
        json_reader::JsonReader reader(f);
        renderer::MapRenderer map_renderer;
        handler::RequestHandler handler(catalogue, map_renderer);
        reader.LoadDataToCatalogue(handler);
        reader.LoadRenderSetting(handler);
        //handler.RenderMap().Render(std::cout);
        reader.PrintStatRequest(handler, std::cout);
    }
    catch (const std::exception& err) {
        std::cerr << err.what();
    }
}

int main() {
    //TestRender();

    catalog::TransportCatalogue catalogue;
    renderer::MapRenderer map_renderer;
    handler::RequestHandler handler(catalogue, map_renderer);
    json_reader::JsonReader reader(std::cin);

    reader.LoadDataToCatalogue(handler);
    reader.LoadRenderSetting(handler);
    reader.PrintStatRequest(handler, std::cout);
}