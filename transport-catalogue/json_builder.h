#pragma once
#include "json.h"

namespace json {

class BuildError : public std::logic_error {
public:
    using logic_error::logic_error;
};

class Builder {
private:
    class ArrayContext;
    class BaseContext;
    class DictKeyContext;
    class DictValueContext;

public:
    Builder()
        :root_()
        , nodes_stack_{ &root_ } {
    }

    ArrayContext StartArray();
    BaseContext EndArray();
    BaseContext EndDict();
    BaseContext Value(Node::Value value);
    DictKeyContext StartDict();
    DictValueContext Key(std::string);
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;

    Node* AddNode(Node::Value&& value);
    Node* GetBackNode() const;

    class BaseContext {
    private:
        Builder& builder_;

    public:
        BaseContext(Builder& builder) : builder_(builder) {}

        BaseContext Value(Node::Value value) {
            return builder_.Value(std::move(value));
        }

        BaseContext EndArray() {
            return builder_.EndArray();
        }
        BaseContext EndDict() {
            return builder_.EndDict();
        }

        ArrayContext StartArray() {
            return builder_.StartArray();
        }

        DictKeyContext StartDict() {
            return builder_.StartDict();
        }

        DictValueContext Key(std::string key) {
            return builder_.Key(std::move(key));
        }

        Node Build() {
            return builder_.Build();
        }

    };

    class ArrayContext : public BaseContext {
    public:
        ArrayContext(BaseContext base) : BaseContext(base) {}

        ArrayContext Value(Node::Value value) {
            return Builder::BaseContext::Value(std::move(value));
        }

        BaseContext EndDict() = delete;
        DictValueContext Key(std::string) = delete;
        Node Build() = delete;
    };

    class DictValueContext : public BaseContext {
    public:
        DictValueContext(BaseContext base) : BaseContext(base) {}

        DictKeyContext Value(Node::Value value) {
            return Builder::BaseContext::Value(std::move(value));
        }

        BaseContext EndArray() = delete;
        BaseContext EndDict() = delete;
        DictValueContext Key(std::string) = delete;
        Node Build() = delete;
    };

    class DictKeyContext : public BaseContext {
    public:
        DictKeyContext(BaseContext base) : BaseContext(base) {}
        ArrayContext StartArray() = delete;
        BaseContext EndArray() = delete;
        BaseContext Value(Node::Value) = delete;
        DictKeyContext StartDict() = delete;
        Node Build() = delete;
    };
};

} // namespace json