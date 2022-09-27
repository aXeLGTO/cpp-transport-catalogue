#include "json_builder.h"
#include "json.h"

#include <algorithm>
#include <stdexcept>

namespace json {

using namespace std;

Builder& Builder::Value(Node::Value value) {
    if (isBuilt) {
        throw logic_error("Object is built");
    }

    if (!root_) {
        root_ = Node{};
        root_->GetValue() = move(value);
    } else if (!nodes_stack_.empty()) {
        auto* node = nodes_stack_.back();

        if (node->IsArray()) {
            Node new_node;
            new_node.GetValue() = move(value);
            node->AsArray().push_back(new_node);
        } else if (!node->IsDict()) {
            node->GetValue() = move(value);
            nodes_stack_.pop_back();
        } else {
            throw logic_error("Key is not set"s);
        }
    } else {
        throw logic_error("Unchained value"s);
    }

    return *this;
}

DictItemContext Builder::StartDict() {
    if (isBuilt) {
        throw logic_error("Object is built");
    }

    if (!root_) {
        root_ = Dict{};
        nodes_stack_.push_back(&root_.value());
    } else if (!nodes_stack_.empty()) {
        auto* node = nodes_stack_.back();
        if (node->IsArray()) {
            node->AsArray().push_back(Dict{});
            nodes_stack_.push_back(&node->AsArray().back());
        } else if (!node->IsDict()) {
            node->GetValue() = Dict{};
        } else {
            throw logic_error("Previous dict is not complete"s);
        }
    } else {
        throw logic_error("Previous node is not finished"s);
    }

    return DictItemContext(*this);
}

Builder& Builder::Key(std::string key) {
    if (isBuilt) {
        throw logic_error("Object is built");
    }

    if (!root_ || nodes_stack_.empty()) {
        throw logic_error("There is not dict for key"s);
    }

    auto* node = nodes_stack_.back();
    node->AsDict().insert({key, Node{}});
    nodes_stack_.push_back(&node->AsDict().at(key));

    return *this;
}

Builder& Builder::EndDict() {
    if (isBuilt) {
        throw logic_error("Object is built");
    }

    if (nodes_stack_.empty() || !nodes_stack_.back()->IsDict()) {
        throw logic_error("There is not dict for close"s);
    }

    nodes_stack_.pop_back();

    return *this;
}

ArrayItemContext Builder::StartArray() {
    if (isBuilt) {
        throw logic_error("Object is built");
    }


    if (!root_) {
        root_ = Array{};
        nodes_stack_.push_back(&root_.value());
    } else if (!nodes_stack_.empty()) {
        auto* node = nodes_stack_.back();
        if (node->IsArray()) {
            node->AsArray().push_back(Array{});
            nodes_stack_.push_back(&node->AsArray().back());
        } else if (!node->IsDict()) {
            node->GetValue() = Array{};
        } else {
            throw logic_error("Key for array value is not set"s);
        }
    } else {
        throw logic_error("Previous node is not finished"s);
    }

    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    if (isBuilt) {
        throw logic_error("Object is built");
    }

    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw logic_error("There is not array for close"s);
    }

    nodes_stack_.pop_back();

    return *this;
}

Node Builder::Build() {
    if (!root_) {
        throw logic_error("Node is empty"s);
    }

    if (!nodes_stack_.empty()) {
        throw logic_error("Node is not complete"s);
    }

    isBuilt = true;

    return *root_;
}

DictItemContext::DictItemContext(Builder& builder) :
    builder_(builder) {
}

KeyValueContext DictItemContext::Key(std::string key) {
    builder_.Key(move(key));
    return KeyValueContext(*this);
}

Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

KeyValueContext::KeyValueContext(DictItemContext& ctx) :
    ctx_(ctx) {
}

DictItemContext& KeyValueContext::Value(Node::Value value) {
    ctx_.builder_.Value(value);
    return ctx_;
}

ArrayItemContext KeyValueContext::StartArray() {
    return ArrayItemContext(ctx_.builder_.StartArray());
}

DictItemContext KeyValueContext::StartDict() {
    return DictItemContext(ctx_.builder_.StartDict());
}

ArrayItemContext::ArrayItemContext(Builder& builder) :
    builder_(builder) {
}

ArrayItemContext& ArrayItemContext::Value(Node::Value value) {
    builder_.Value(value);
    return *this;
}

ArrayItemContext& ArrayItemContext::StartArray() {
    builder_.StartArray();
    return *this;
}

Builder& ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

DictItemContext ArrayItemContext::StartDict() {
    return DictItemContext(builder_.StartDict());
}

} // namespace json
