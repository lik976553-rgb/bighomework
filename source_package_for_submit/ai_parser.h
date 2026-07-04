#pragma once

#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct AiFoodInput {
    string type;
    string name;
    double weight = 0.0;

    bool isValid() const {
        return !type.empty() && !name.empty() && weight > 0.0;
    }
};

class AiFoodParser {
public:
    static vector<AiFoodInput> parse(const string& jsonText, string* errorMessage = nullptr) {
        try {
            AiFoodParser parser(jsonText);
            vector<AiFoodInput> result = parser.parseFoodList();
            if (errorMessage) errorMessage->clear();
            return result;
        } catch (const exception& e) {
            if (errorMessage) *errorMessage = e.what();
            return {};
        }
    }

private:
    string text;
    size_t pos = 0;

    explicit AiFoodParser(const string& jsonText) : text(jsonText) {}

    vector<AiFoodInput> parseFoodList() {
        vector<AiFoodInput> foods;
        skipSpaces();

        if (match('[')) {
            skipSpaces();
            if (match(']')) return foods;

            while (true) {
                AiFoodInput food = parseFoodObject();
                if (food.isValid()) foods.push_back(food);

                skipSpaces();
                if (match(']')) break;
                expect(',');
            }
        } else {
            while (!isEnd()) {
                AiFoodInput food = parseFoodObject();
                if (food.isValid()) foods.push_back(food);

                skipSpaces();
                if (isEnd()) break;
                expect(',');
            }
        }

        skipSpaces();
        if (!isEnd()) {
            throw runtime_error("Unexpected content after JSON food list.");
        }
        return foods;
    }

    AiFoodInput parseFoodObject() {
        AiFoodInput food;
        expect('{');

        skipSpaces();
        if (match('}')) return food;

        while (true) {
            string key = parseString();
            expect(':');
            skipSpaces();

            if (key == "type") {
                food.type = parseString();
            } else if (key == "name") {
                food.name = parseString();
            } else if (key == "weight") {
                food.weight = parseNumber();
            } else {
                skipValue();
            }

            skipSpaces();
            if (match('}')) break;
            expect(',');
        }

        return food;
    }

    string parseString() {
        skipSpaces();
        expect('"');

        string result;
        while (!isEnd()) {
            char ch = text[pos++];
            if (ch == '"') return result;

            if (ch == '\\') {
                if (isEnd()) throw runtime_error("Invalid string escape.");
                char escaped = text[pos++];
                switch (escaped) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += escaped; break;
                }
            } else {
                result += ch;
            }
        }

        throw runtime_error("Unclosed string.");
    }

    double parseNumber() {
        skipSpaces();
        size_t start = pos;

        if (peek() == '-') pos++;
        while (isdigit((unsigned char)peek())) pos++;
        if (peek() == '.') {
            pos++;
            while (isdigit((unsigned char)peek())) pos++;
        }
        if (peek() == 'e' || peek() == 'E') {
            pos++;
            if (peek() == '+' || peek() == '-') pos++;
            while (isdigit((unsigned char)peek())) pos++;
        }

        if (start == pos) throw runtime_error("Expected number.");
        return stod(text.substr(start, pos - start));
    }

    void skipValue() {
        skipSpaces();
        char ch = peek();

        if (ch == '"') {
            parseString();
        } else if (ch == '{') {
            skipNested('{', '}');
        } else if (ch == '[') {
            skipNested('[', ']');
        } else if (isdigit((unsigned char)ch) || ch == '-') {
            parseNumber();
        } else {
            while (!isEnd() && !isspace((unsigned char)peek()) &&
                   peek() != ',' && peek() != '}' && peek() != ']') {
                pos++;
            }
        }
    }

    void skipNested(char openChar, char closeChar) {
        expect(openChar);
        int depth = 1;
        while (!isEnd() && depth > 0) {
            char ch = text[pos];
            if (ch == '"') {
                parseString();
            } else {
                pos++;
                if (ch == openChar) depth++;
                if (ch == closeChar) depth--;
            }
        }

        if (depth != 0) throw runtime_error("Unclosed nested JSON value.");
    }

    void skipSpaces() {
        while (!isEnd() && isspace((unsigned char)text[pos])) pos++;
    }

    bool match(char expected) {
        skipSpaces();
        if (peek() != expected) return false;
        pos++;
        return true;
    }

    void expect(char expected) {
        skipSpaces();
        if (peek() != expected) {
            string message = "Expected '";
            message += expected;
            message += "'.";
            throw runtime_error(message);
        }
        pos++;
    }

    char peek() const {
        return isEnd() ? '\0' : text[pos];
    }

    bool isEnd() const {
        return pos >= text.size();
    }
};
