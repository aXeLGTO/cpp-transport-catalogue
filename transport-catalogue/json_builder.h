#pragma once
#include "json.h"

#include <optional>
#include <string_view>
#include <vector>

namespace json {

class Builder {
public:
    Builder& Value(Node::Value value);

    class DictItemContext StartDict();
    Builder& Key(std::string key);
    Builder& EndDict();

    class ArrayItemContext StartArray();
    Builder& EndArray();

    Node Build();

private:
    std::optional<Node> root_;

    bool isBuilt = false;

    std::vector<Node*> nodes_stack_;
};

class DictItemContext {
public:
    explicit DictItemContext(Builder& builder);

    class KeyValueContext Key(std::string key);
    Builder& EndDict();

private:
    friend class KeyValueContext;
    Builder& builder_;
};

class KeyValueContext {
public:
    explicit KeyValueContext(DictItemContext& ctx);

    DictItemContext& Value(Node::Value value);

    class ArrayItemContext StartArray();
    DictItemContext StartDict();

private:
    DictItemContext& ctx_;
};

class ArrayItemContext {
public:
    explicit ArrayItemContext(Builder& builder);

    ArrayItemContext& Value(Node::Value value);

    ArrayItemContext& StartArray();
    DictItemContext StartDict();

    Builder& EndArray();

private:
    Builder& builder_;
};

} // namespace json
