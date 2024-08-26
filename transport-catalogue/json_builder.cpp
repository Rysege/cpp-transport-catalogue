#include "json_builder.h"

namespace json {

using namespace std::literals;

Node* Builder::AddNode(Node::Value&& value) {
    Node* ptr = GetBackNode();

    if (ptr->IsArray()) {
        ptr = &std::get<Array>(ptr->GetValue()).emplace_back(std::move(value));
    }
    else if (ptr->IsDict()) {
        throw BuildError("Key() was expected"s);
    }
    else {
        ptr = &(*ptr = std::move(value));
        nodes_stack_.pop_back();
    }
    return ptr;
}

Node* Builder::GetBackNode() const {
    if (nodes_stack_.empty()) {
        throw BuildError("Attempt to change finalized JSON"s);
    }
    return nodes_stack_.back();
}

Builder::BaseContext Builder::Value(Node::Value value) {
    AddNode(std::move(value));
    return *this;
}

Builder::ArrayContext Builder::StartArray() {
    nodes_stack_.push_back(AddNode(Array{}));
    return ArrayContext{ *this };
}

Builder::BaseContext Builder::EndArray() {
    if (!GetBackNode()->IsArray()) {
        throw BuildError("EndArray() outside an array"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder::DictKeyContext Builder::StartDict() {
    nodes_stack_.push_back(AddNode(Dict{}));
    return DictKeyContext{ *this };
}

Builder::BaseContext Builder::EndDict() {
    if (!GetBackNode()->IsDict()) {
        throw BuildError("EndDict() outside a dict"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder::DictValueContext Builder::Key(std::string str) {
    Node* node = GetBackNode();
    if (!node->IsDict()) {
        throw BuildError("Key() outside a dict"s);
    }

    auto it = std::get<Dict>(node->GetValue()).emplace(std::move(str), Node{}).first;
    nodes_stack_.push_back(&it->second);
    return DictValueContext{ *this };
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw BuildError("Attempt to build JSON which isn't finalized"s);
    }
    return std::move(root_);
}

} // namespace json