#include "json.h"

namespace json {

    namespace {
        using namespace std::literals;

        Node LoadNode(std::istream& input);
        Node LoadString(std::istream& input);

        std::string LoadLiteral(std::istream& input) {
            std::string s;
            while (std::isalpha(input.peek())) {
                s.push_back(static_cast<char>(input.get()));
            }
            return s;
        }

        Node LoadArray(std::istream& input) {
            std::vector<Node> result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Array parsing error"s);
            }
            return Node(std::move(result));
        }

        Node LoadDict(std::istream& input) {
            Dict dict;

            for (char c; input >> c && c != '}';) {
                if (c == '"') {
                    std::string key = LoadString(input).AsString();
                    if (input >> c && c == ':') {
                        if (dict.find(key) != dict.end()) {
                            throw ParsingError("Duplicate key '"s + key + "' have been found");
                        }
                        dict.emplace(std::move(key), LoadNode(input));
                    }
                    else {
                        throw ParsingError(": is expected but '"s + c + "' has been found"s);
                    }
                }
                else if (c != ',') {
                    throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
                }
            }
            if (!input) {
                throw ParsingError("Dictionary parsing error"s);
            }
            return Node(std::move(dict));
        }

        Node LoadString(std::istream& input) {
            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
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
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    s.push_back(ch);
                }
                ++it;
            }

            return Node(std::move(s));
        }

        Node LoadBool(std::istream& input) {
            const auto s = LoadLiteral(input);
            if (s == "true"sv) {
                return Node{ true };
            }
            else if (s == "false"sv) {
                return Node{ false };
            }
            else {
                throw ParsingError("Failed to parse '"s + s + "' as bool"s);
            }
        }

        Node LoadNull(std::istream& input) {
            if (auto literal = LoadLiteral(input); literal == "null"sv) {
                return Node{ nullptr };
            }
            else {
                throw ParsingError("Failed to parse '"s + literal + "' as null"s);
            }
        }

        Node LoadNumber(std::istream& input) {
            std::string parsed_num;

            // ????????? ? parsed_num ????????? ?????? ?? input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // ????????? ???? ??? ????? ???? ? parsed_num ?? input
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
            // ?????? ????? ????? ?????
            if (input.peek() == '0') {
                read_char();
                // ????? 0 ? JSON ?? ????? ???? ?????? ?????
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // ?????? ??????? ????? ?????
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // ?????? ???????????????? ????? ?????
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
                    // ??????? ??????? ????????????? ?????? ? int
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                        // ? ?????? ???????, ????????, ??? ????????????
                        // ??? ???? ????????? ????????????? ?????? ? double
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(std::istream& input) {
            char c;
            if (!(input >> c)) {
                throw ParsingError("Unexpected EOF"s);
            }
            switch (c) {
            case '[':
                return LoadArray(input);
            case '{':
                return LoadDict(input);
            case '"':
                return LoadString(input);
            case 't':
                // ??????? [[fallthrough]] (???????????) ?????? ?? ??????, ? ????????
                // ?????????? ??????????? ? ????????, ??? ????? ??????????? ???? ?????????
                // ????????? ??????? ? ?????????? ????????? ????? case, ? ?? ???????? ?????
                // ???????? break, return ??? throw.
                // ? ?????? ??????, ???????? t ??? f, ????????? ? ??????? ????????
                // ????????? true ???? false
                [[fallthrough]];
            case 'f':
                input.putback(c);
                return LoadBool(input);
            case 'n':
                input.putback(c);
                return LoadNull(input);
            default:
                input.putback(c);
                return LoadNumber(input);
            }
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

            PrintContext Indented() const {
                return { out, indent_step, indent_step + indent };
            }
        };

        void PrintNode(const Node& value, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintString(const std::string& value, std::ostream& out) {
            out.put('"');
            for (const char c : value) {
                switch (c) {
                case '\r':
                    out << "\\r"sv;
                    break;
                case '\n':
                    out << "\\n"sv;
                    break;
                case '"':
                    // ??????? " ? \ ????????? ??? \" ??? \\, ??????????????
                    [[fallthrough]];
                case '\\':
                    out.put('\\');
                    [[fallthrough]];
                default:
                    out.put(c);
                    break;
                }
            }
            out.put('"');
        }

        template <>
        void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
            PrintString(value, ctx.out);
        }

        template <>
        void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
            ctx.out << "null"sv;
        }

        // ? ???????????? ??????? PrintValue ??? ???? bool ???????? value ??????????
        // ?? ??????????? ??????, ??? ? ? ???????? ???????.
        // ? ???????? ???????????? ????? ???????????? ??????????:
        // void PrintValue(bool value, const PrintContext& ctx);
        template <>
        void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
            ctx.out << (value ? "true"sv : "false"sv);
        }

        template <>
        void PrintValue<Array>(const Array& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;
            out << "[\n"sv;
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const Node& node : nodes) {
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
            out.put(']');
        }

        template <>
        void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;
            out << "{\n"sv;
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& [key, node] : nodes) {
                if (first) {
                    first = false;
                }
                else {
                    out << ",\n"sv;
                }
                inner_ctx.PrintIndent();
                PrintString(key, ctx.out);
                out << ": "sv;
                PrintNode(node, inner_ctx);
            }
            out.put('\n');
            ctx.PrintIndent();
            out.put('}');
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit(
                [&ctx](const auto& value) {
                    PrintValue(value, ctx);
                },
                node.GetValue());
        }

    }  // namespace

    Document Load(std::istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), PrintContext{ output });
    }

}  // namespace json 

/*#include "json.h"
using namespace std;
namespace json {
    namespace {
        Node LoadNode(istream& input);
        Node LoadArray(istream& input) {
            Array result;
            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Array parsing error");
            }
            return Node(move(result));
        }
        Node LoadString(istream& input) {
            string line = "";
            char c;
            while (true) {
                if (!input) {
                    throw ParsingError("String parsing error");
                }
                input.get(c);
                if (c == '"') {
                    break;
                }
                else {
                    if (c == '\\') {
                        input.get(c);
                        if (c == '\\' || c == '\"') {
                            line += c;
                            continue;
                        }
                        else {
                            switch (c) {
                            case 'b': line += '\b'; break;
                            case 'f': line += '\f'; break;
                            case 'n': line += '\n'; break;
                            case 'r': line += '\r'; break;
                            case 't': line += '\t'; break;
                            }
                            continue;
                        }
                    }
                    line += c;
                }
            }
            return Node(move(line));
        }
        Node LoadDict(istream& input) {
            Dict result;
            if (input.peek() < 0) {
                throw ParsingError("there is no second bracket");
            }
            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }
                string key = LoadString(input).AsString();
                input >> c;
                result.insert({ move(key), LoadNode(input) });
            }
            return Node(move(result));
        }
        using Number = std::variant<int, double>;
        Node LoadNumber(std::istream& input) {
            using namespace std::literals;
            std::string parsed_num;
            // ????????? ? parsed_num ????????? ?????? ?? input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };
            // ????????? ???? ??? ????? ???? ? parsed_num ?? input
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
            // ?????? ????? ????? ?????
            if (input.peek() == '0') {
                read_char();
                // ????? 0 ? JSON ?? ????? ???? ?????? ?????
            }
            else {
                read_digits();
            }
            bool is_int = true;
            // ?????? ??????? ????? ?????
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }
            // ?????? ???????????????? ????? ?????
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
                    // ??????? ??????? ????????????? ?????? ? int
                    try {
                        return Node{ std::stoi(parsed_num) };
                    }
                    catch (...) {
                        // ? ?????? ???????, ????????, ??? ????????????
                        // ??? ???? ????????? ????????????? ?????? ? double
                    }
                }
                return Node{ std::stod(parsed_num) };
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }
        Node LoadNode(istream& input) {
            char c;
            input >> c;
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
                if (c == 'n') {
                    std::string valid_str = "n"s;
                    for (int i = 0; i < 3; i++) {
                        input >> c;
                        if (!input) {
                            throw ParsingError("null parsing error");
                        }
                        valid_str += c;
                    }
                    if (valid_str != "null") {
                        throw ParsingError("null parsing error");
                    }
                    return Node{};
                }
                else if (c == 't') {
                    std::string valid_str = "t"s;
                    for (int i = 0; i < 3; i++) {
                        input >> c;
                        if (!input) {
                            throw ParsingError("null parsing error");
                        }
                        valid_str += c;
                    }
                    if (valid_str != "true") {
                        throw ParsingError("true parsing error");
                    }
                    return Node{ true };
                }
                else if (c == 'f') {
                    std::string valid_str = "f"s;
                    for (int i = 0; i < 4; i++) {
                        input >> c;
                        if (!input) {
                            throw ParsingError("null parsing error");
                        }
                        valid_str += c;
                    }
                    if (valid_str != "false") {
                        throw ParsingError("false parsing error");
                    }
                    return Node{ false };
                }
                else if (c == 45 || (c >= 48 && c <= 57)) {
                    input.putback(c);
                    return LoadNumber(input);
                }
                else {
                    throw ParsingError("invalid request"s);
                }
            }
        }
    }  // namespace
    //Node::Node(Array array)
    //    : as_array_(move(array)) {
    //}
    //Node::Node(Dict map)
    //    : as_map_(move(map)) {
    //}
    //Node::Node(int value)
    //    : as_int_(value) {
    //}
    //Node::Node(string value)
    //    : as_string_(move(value)) {
    //}
    //---------------- Node -----------------
    Node::Node() : value_(nullptr) {
    }
    Node::Node(std::nullptr_t) : value_(nullptr) {
    }
    Node::Node(Array val) : value_(std::move(val)) {
    }
    Node::Node(Dict val) : value_(std::move(val)) {
    }
    Node::Node(bool val) : value_(val) {
    }
    Node::Node(int val) : value_(val) {
    }
    Node::Node(double val) : value_(val) {
    }
    Node::Node(std::string val) : value_(std::move(val)) {
    }
    bool Node::IsNull() const {
        return holds_alternative<std::nullptr_t>(value_);
    }
    bool Node::IsArray() const {
        return holds_alternative<Array>(value_);
    }
    bool Node::IsDict() const {
        return holds_alternative<Dict>(value_);
    }
    bool Node::IsBool() const {
        return holds_alternative<bool>(value_);
    }
    bool Node::IsInt() const {
        return holds_alternative<int>(value_);
    }
    bool Node::IsDouble() const {
        return IsPureDouble() || IsInt();
    }
    bool Node::IsPureDouble() const {
        return holds_alternative<double>(value_);
    }
    bool Node::IsString() const {
        return holds_alternative<std::string>(value_);
    }
    //const Array& Node::AsArray() const {
    //    return as_array_;
    //}
    //const Dict& Node::AsMap() const {
    //    return as_map_;
    //}
    //int Node::AsInt() const {
    //    return as_int_;
    //}
    //const string& Node::AsString() const {
    //    return as_string_;
    //}
    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("no array");
        }
        return std::get<Array>(value_);
    }
    const Dict& Node::AsDict() const {
        if (!IsDict()) {
            throw  std::logic_error("no map");
        }
        return std::get<Dict>(value_);
    }
    bool Node::AsBool() const {
        if (!IsBool()) {
            throw  std::logic_error("no bool");
        }
        return std::get<bool>(value_);
    }
    int Node::AsInt() const {
        if (!IsInt()) {
            throw  std::logic_error("no int");
        }
        return std::get<int>(value_);
    }
    double Node::AsDouble() const {
        if (!IsDouble()) {
            throw  std::logic_error("no double");
        }
        if (IsPureDouble()) {
            return std::get<double>(value_);
        }
        return static_cast<double>(std::get<int>(value_));
    }
    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("no string");
        }
        return std::get<std::string>(value_);
    }
    bool operator==(const json::Node& lhs, const json::Node& rhs) {
        return lhs.value_ == rhs.value_;
    }
    bool operator !=(const json::Node& lhs, const json::Node& rhs) {
        return !(lhs == rhs);
    }
    //--------------------- Document ---------------------
    Document::Document(Node root)
        : root_(move(root)) {
    }
    const Node& Document::GetRoot() const {
        return root_;
    }
    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }
    void Print(const Document& doc, std::ostream& output) {
        if (doc.GetRoot().IsNull()) {
            output << "null";
        }
        else if (doc.GetRoot().IsPureDouble()) {
            output << doc.GetRoot().AsDouble();
        }
        else if (doc.GetRoot().IsInt()) {
            output << doc.GetRoot().AsInt();
        }
        else if (doc.GetRoot().IsString()) {
            std::string str = doc.GetRoot().AsString();
            output << '\"';
            for (auto c = str.cbegin(); c != str.cend(); c++) {
                switch (*c) {
                case '\"': output << "\\\""; break;
                case '\\': output << "\\\\"; break;
                case '\b': output << "\\b"; break;
                case '\f': output << "\\f"; break;
                case '\n': output << "\\n"; break;
                case '\r': output << "\\r"; break;
                case '\t': output << "\\t"; break;
                default:
                    if ('\x00' <= *c && *c <= '\x1f') {
                        output << "\\u"
                            << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                    }
                    else {
                        output << *c;
                    }
                }
            }
            output << '\"';
        }
        else if (doc.GetRoot().IsBool()) {
            doc.GetRoot().AsBool() == true ? output << "true" : output << "false";
        }
        else if (doc.GetRoot().IsArray()) {
            auto Arr = doc.GetRoot().AsArray();
            output << '[';
            for (auto It = Arr.begin(); It < Arr.end(); It++) {
                if (It == Arr.begin()) {
                    Print(Document{ *It }, output);
                }
                else {
                    output << ", ";
                    Print(Document{ *It }, output);
                }
            }
            output << ']';
        }
        else if (doc.GetRoot().IsDict()) {
            auto Map = doc.GetRoot().AsDict();
            output << '{';
            for (auto it = Map.begin(); it != Map.end(); it++) {
                if (it == Map.begin()) {
                    output << '"' << it->first << "\": ";
                    Print(Document{ it->second }, output);
                }
                else {
                    output << ", \"" << it->first << "\": ";
                    Print(Document{ it->second }, output);
                }
            }
            output << '}';
        }
    }
    bool operator==(const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }
    bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }
}  // namespace json */