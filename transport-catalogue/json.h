#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <variant>
#include <sstream>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final
    : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {

public:
    using variant::variant;

    bool IsNull() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsBool() const;
    int AsInt() const;
    double AsDouble() const;
    double AsPureDouble() const;
    std::string AsString() const;
    Array AsArray() const;
    Dict AsMap() const;

    const std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>& GetValue() const { return *this; }
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

bool operator==(const Document& lhs, const Document& rhs);
bool operator!=(const Document& lhs, const Document& rhs);

Document Load(std::istream& input);

Node LoadNode(std::istream& input);
Node LoadArray(std::istream& input);
Node LoadString(std::istream& input);
Node LoadDict(std::istream& input);
Node LoadNumber(std::istream& input);

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
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
        return {out, indent_step, indent_step + indent};
    }
};

// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintValue(std::nullptr_t, const PrintContext& ctx);
void PrintValue(bool value, const PrintContext& ctx);
void PrintValue(std::string value, const PrintContext& ctx);
void PrintValue(Array value, const PrintContext& ctx);
void PrintValue(Dict value, const PrintContext& ctx);

void PrintNode(const Node& node, const PrintContext& ctx);

void Print(const Document& doc, std::ostream& output);

}  // namespace json
