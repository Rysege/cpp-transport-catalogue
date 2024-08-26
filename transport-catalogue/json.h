#pragma once
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final
    : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:
    using variant::variant;
    using Value = variant;

    Node(Value value) : variant(std::move(value)) {}

    bool IsNull() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsPureDouble() const;
    bool IsDouble() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsDict() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsDict() const;

    bool operator==(const Node& rhs) const;
    const Value& GetValue() const;
    Value& GetValue();
};

inline bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;

private:
    Node root_;
};

inline bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return { out, indent_step, indent_step + indent };
    }
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

namespace output {

template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

} // namespace output
} // namespace json