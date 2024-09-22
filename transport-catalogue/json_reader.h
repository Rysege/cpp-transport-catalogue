#pragma once
#include "json.h"
#include "request_handler.h"

#include <algorithm>

namespace json_reader {

class RequestError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
    RequestError() : runtime_error("Invalid request") {}
};

class JsonReader {
public:
    JsonReader(std::istream& in) : data_(json::Load(in)) {}

    void ReadJson(std::istream& in);
    void LoadData(handler::RequestHandler& rh) const;
    void PrintStatRequest(handler::RequestHandler& rh, std::ostream& out) const;
    renderer::RenderSettings LoadRenderSettings() const;
    routemap::RoutingSettings LoadRoutingSettings() const;

private:
    json::Document data_;

    const json::Node& GetNodeRequest(const std::string& name) const;
    json::Document ProcessStatRequests(handler::RequestHandler& handler) const;
};

template <typename ReturnType, typename Iterator, typename Lambda>
ReturnType ConvertTo(Iterator first, Iterator last, Lambda lambda) {
    ReturnType result{};
    std::transform(first, last, std::inserter(result, end(result)), lambda);
    return result;
}

template<typename ReturnType, typename Func>
ReturnType TryCatch(Func func, const json::Dict& dict) {
    try {
        return func(dict);
    }
    catch (std::out_of_range const&) {
        throw RequestError();
    }
    catch (...) {
        throw;
    }
}

} // namespace json_reader
