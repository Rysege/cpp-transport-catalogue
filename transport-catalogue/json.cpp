#include "json.h"

#include <numeric>

namespace json {

using namespace std::literals;

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsDouble() const {
    return IsInt() || IsPureDouble();
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsDict() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("Is not int"s);
    }
    return std::get<int>(*this);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Is not bool"s);
    }
    return std::get<bool>(*this);
}

double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("Is not double"s);
    }
    return IsPureDouble() ? std::get<double>(*this) : AsInt();
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Is not string"s);
    }
    return std::get<std::string>(*this);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Is not Array"s);
    }
    return std::get<Array>(*this);
}

const Dict& Node::AsDict() const {
    if (!IsDict()) {
        throw std::logic_error("Is not Dict"s);
    }
    return std::get<Dict>(*this);
}

bool Node::operator==(const Node& rhs) const {
    return this->GetValue() == rhs.GetValue();
}

const Node::Value& Node::GetValue() const {
    return *this;
}

Node::Value& Node::GetValue() {
    return *this;
}

namespace detail {

Node LoadNode(std::istream& input);

Node LoadToken(std::istream& input) {
    std::string token;
    while (std::isalpha(input.peek())) {
        token.push_back(std::tolower(static_cast<char>(input.get())));
    }
    if (token.empty()) {
        throw ParsingError("Parsing error"s);
    }

    if (token == "true"s) {
        return Node{ true };
    }
    else if (token == "false"s) {
        return Node{ false };
    }
    else if (token == "null"s) {
        return Node{ nullptr };
    }
    throw ParsingError("Invalid token: "s + token);
}

Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    }
    else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node{ std::stoi(parsed_num) };
            }
            catch (...) {
             // В случае неудачи, например, при переполнении,
             // код ниже попробует преобразовать строку в double
            }
        }
        return Node{ std::stod(parsed_num) };
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        }
        else if (ch == '\\') {
         // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
         // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        }
        else {
         // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(move(s));
}

Node LoadArray(std::istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (!input) {
        throw ParsingError("Array parsing error"s);
    }
    return Node(move(result));
}

Node LoadDict(std::istream& input) {
    Dict result;

    for (char c; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }
        std::string key;
        if (c == '"') {
            key = LoadString(input).AsString();
        }
        if (key.empty() || !(input >> c) || c != ':') {
            throw ParsingError("Dictionary key/value parsing error"s);
        }
        result.insert({ move(key), LoadNode(input) });
    }
    if (!input) {
        throw ParsingError("Dictionary parsing error"s);
    }
    return Node(move(result));
}

Node LoadNode(std::istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("The unexpected end of the stream"s);
    }

    if (c == '[') {
        return LoadArray(input);
    }
    else if (c == '{') {
        return LoadDict(input);
    }
    else if (c == '"') {
        return LoadString(input);
    }
    else {
        input.putback(c);
        if (c == '-' || std::isdigit(c)) {
            return LoadNumber(input);
        }
        return LoadToken(input);
    }
}

} // namespace detail

namespace output {

void PrintNode(const Node& node, const PrintContext& ctx);

void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null";
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << std::boolalpha << value;
}

void PrintValue(const std::string& value, const PrintContext& ctx) {
    static const std::map<char, std::string_view> ctos = {
        { '\n', "\\n"sv },
        { '\r', "\\r"sv },
        { '\t', "\\t"sv },
        { '\"', "\\\""sv },
        { '\\', "\\\\"sv }
    };
    auto& out = ctx.out;
    out.put('"');
    for (const char c : value) {
        if (auto it = ctos.find(c); it != ctos.end()) {
            out << it->second;
        }
        else {
            out.put(c);
        }
    }
    out.put('"');
}

void PrintValue(const Array& value, const PrintContext& ctx) {
    auto& out = ctx.out;
    out.put('[');
    if (!value.empty()) {
        out.put('\n');
        auto inner_ctx = ctx.Indented();
        bool first = true;
        for (auto& node : value) {
            if (first) {
                first = false;
            }
            else {
                out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintNode(node, inner_ctx);
        }
        out.put('\n');
        ctx.PrintIndent();
    }
    out.put(']');
}

void PrintValue(const Dict& value, const PrintContext& ctx) {
    auto& out = ctx.out;
    out.put('{');
    if (!value.empty()) {
        out.put('\n');
        bool first = true;
        auto inner_ctx = ctx.Indented();
        int max_size = std::accumulate(value.begin(), value.end(), 0,
            [](size_t m, const std::pair<std::string, Node>& p) {return std::max(m, p.first.size()); });

        for (auto& [key, node] : value) {
            if (first) {
                first = false;
            }
            else {
                out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintNode(key, ctx);
            out << std::string(max_size - key.size(), ' ') << ": "sv;
            PrintNode(node, inner_ctx);
        }
        out.put('\n');
        ctx.PrintIndent();
    }
    out.put('}');
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value) { PrintValue(value, ctx); },
        node.GetValue());
}

} // namespace output

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const {
    return root_ == other.root_;
}

Document Load(std::istream& input) {
    return Document{ detail::LoadNode(input) };
}

void Print(const Document& doc, std::ostream& out) {
    output::PrintNode(doc.GetRoot(), PrintContext{ out });
}

} // namespace json