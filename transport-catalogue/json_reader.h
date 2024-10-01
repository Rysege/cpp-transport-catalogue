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
    JsonReader(catalog::TransportCatalogue& db, std::istream& in);

    void PrintStatRequest(std::ostream& out) const;

private:
    json::Document data_;
    handler::RequestHandler handler_;

    void LoadData() const;
    const json::Node& GetNodeRequest(const std::string& name) const;
    json::Document ProcessStatRequests() const;
    renderer::RenderSettings ParseRenderSettings() const;
    routemap::RoutingSettings ParseRoutingSettings() const;
};

template <typename ReturnType, typename Iterator, typename Lambda>
auto ConvertTo(Iterator first, Iterator last, Lambda lambda) {
    ReturnType result{};
    std::transform(first, last, std::inserter(result, end(result)), lambda);
    return result;
}

template<typename Func>
auto TryCatch(Func func, const json::Dict& dict) {
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
