#pragma once
#include "json.h"
#include "request_handler.h"

#include <algorithm>
#include <functional>

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
    void LoadDataToCatalogue(handler::RequestHandler& handler) const;
    void LoadRenderSetting(handler::RequestHandler& handler) const;
    void PrintStatRequest(handler::RequestHandler& handler, std::ostream& out) const;

private:
    json::Document data_;

    const json::Node& GetNodeRequest(const std::string& name) const;
    json::Document ProcessStatRequests(handler::RequestHandler& handler) const;
};

template <typename ReturnType, typename Container, typename Lambda>
ReturnType ConvertTo(const Container& container, Lambda lambda) {
    ReturnType result;
    std::transform(begin(container), end(container)
        , std::inserter(result, end(result)), lambda);
    return result;
}

template<typename ReturnType>
ReturnType TryCatch(std::function<ReturnType(const json::Dict&)> func, const json::Dict& dict) {
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
