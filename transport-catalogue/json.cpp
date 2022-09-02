#include "json.h"
#include <cstddef>
#include <variant>
#include <stdexcept>
#include <iomanip>

using namespace std;
using namespace std::literals;

namespace json {

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c;) {
        if (c == ']') {
            return Node(move(result));
        }
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    throw ParsingError("Failed to read array from stream"s);
}

Node LoadNumber(istream& input) {
    string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (isdigit(input.peek())) {
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
    } else {
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
                return Node(stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node(stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {
    using namespace std::literals;

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
        } else if (ch == '\\') {
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
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(move(s));
}

Node LoadDict(istream& input) {
    Dict result;

    for (char c; input >> c;) {
        if (c == ',') {
            input >> c;
        }
        if (c == '}') {
            return Node(move(result));
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    throw ParsingError("Failed to read dict from stream"s);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == '-' || c == '+' || isdigit(c)) {
        input.putback(c);
        return LoadNumber(input);
    } else {
        input.putback(c);
        string value;
        input >> setw(4) >> value;

        if (value == "null"s) {
            return {};
        } else if (value == "true"s) {
            return {true};
        } else {
            value.push_back(input.get());
            if (value == "false"s) {
                return {false};
            } else {
                throw ParsingError("An unknown value"s);
            }
        }
    }
}

Node::Node(nullptr_t) {
}

Node::Node(bool value) :
    value_(value) {
}

Node::Node(int value)
    : value_(value) {
}

Node::Node(double value)
    : value_(value) {
}

Node::Node(string value)
    : value_(move(value)) {
}

Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
    return holds_alternative<double>(value_) || holds_alternative<int>(value_);;
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsString() const {
    return holds_alternative<string>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

int Node::AsBool() const {
    if (auto* val = get_if<bool>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not bool"s);
}

int Node::AsInt() const {
    if (auto* val = get_if<int>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not int"s);
}

double Node::AsDouble() const {
    if (auto* val = get_if<double>(&value_)) {
        return *val;
    } else if (auto* val = get_if<int>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not double"s);
}

double Node::AsPureDouble() const {
    if (auto* val = get_if<double>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not pure double"s);
}

string Node::AsString() const {
    if (auto* val = get_if<string>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not string"s);
}

Array Node::AsArray() const {
    if (auto* val = get_if<Array>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not array"s);
}

Dict Node::AsMap() const {
    if (auto* val = get_if<Dict>(&value_)) {
        return *val;
    }
    throw logic_error("Node value is not dict"s);
}

bool operator==(const Node& lhs, const Node& rhs) {
    return lhs.GetValue() == rhs.GetValue();
}

bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}

bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot().GetValue() == rhs.GetRoot().GetValue();
}

bool operator!=(const Document& lhs, const Document& rhs) {
    return !(lhs == rhs);
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << (value ? "true"sv : "false"sv);
}

void PrintValue(string value, const PrintContext& ctx) {
    string str;
    for (auto c : value) {
        switch (c) {
            case '\n':
                str.push_back('\\');
                str.push_back('n');
                break;
            case '\r':
                str.push_back('\\');
                str.push_back('r');
                break;
            case '"':
            case '\\':
                str.push_back('\\');
                str.push_back(c);
                break;
            default:
                str.push_back(c);
        }
    }
    ctx.out << "\""s << str << "\""s;
}

void PrintValue(Array value, const PrintContext& ctx) {
    ctx.out << "[\n";
    bool isFirst = true;
    const auto intended = ctx.Indented();
    for (const auto& v : value) {
        if (!isFirst) {
            ctx.out << ",\n";
        }
        isFirst = false;
        intended.PrintIndent();
        PrintNode(v, intended);
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << ']';
}

void PrintValue(Dict value, const PrintContext& ctx) {
    ctx.out << "{\n";
    bool isFirst = true;
    const auto intended = ctx.Indented();
    for (const auto& [key, val] : value) {
        if (!isFirst) {
            ctx.out << ",\n";
        }
        isFirst = false;
        intended.PrintIndent();
        ctx.out << '"' << key << "\": "s;
        PrintNode(val, intended);
    }
    ctx.out << '\n';
    ctx.PrintIndent();
    ctx.out << '}';
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value){ PrintValue(value, ctx); },
        node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), {output});
}

}  // namespace json
