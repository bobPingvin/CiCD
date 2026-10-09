#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cctype>

enum class JsonType { Null, Boolean, Number, String, Array, Object };

class JsonValue {
public:
    JsonType type = JsonType::Null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    bool is_null() const { return type == JsonType::Null; }
    bool is_bool() const { return type == JsonType::Boolean; }
    bool is_number() const { return type == JsonType::Number; }
    bool is_string() const { return type == JsonType::String; }
    bool is_array() const { return type == JsonType::Array; }
    bool is_object() const { return type == JsonType::Object; }

    const std::string& as_string() const {
        if (!is_string()) throw std::runtime_error("JSON: Value is not a string");
        return str_val;
    }
    double as_number() const {
        if (!is_number()) throw std::runtime_error("JSON: Value is not a number");
        return num_val;
    }
    const std::map<std::string, JsonValue>& as_object() const {
        if (!is_object()) throw std::runtime_error("JSON: Value is not an object");
        return obj_val;
    }
    const std::vector<JsonValue>& as_array() const {
        if (!is_array()) throw std::runtime_error("JSON: Value is not an array");
        return arr_val;
    }
    bool contains(const std::string& key) const {
        return is_object() && obj_val.find(key) != obj_val.end();
    }
    const JsonValue& get(const std::string& key) const {
        auto it = obj_val.find(key);
        if (it == obj_val.end()) throw std::runtime_error("JSON key not found: " + key);
        return it->second;
    }
    std::string get_string_or(const std::string& key, const std::string& def) const {
        if (contains(key) && get(key).is_string()) return get(key).as_string();
        return def;
    }
};

class JsonParser {
    std::string src;
    size_t pos = 0;

    void skip_whitespace() {
        while (pos < src.size() && (std::isspace(src[pos]) || src[pos] == '\r')) {
            pos++;
        }
    }
    char peek() { skip_whitespace(); return pos < src.size() ? src[pos] : '\0'; }
    char get() { skip_whitespace(); return pos < src.size() ? src[pos++] : '\0'; }

    std::string parse_string() {
        if (get() != '"') throw std::runtime_error("Expected '\"'");
        std::string res;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return res;
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                if (esc == 'n') res += '\n';
                else if (esc == 't') res += '\t';
                else if (esc == 'r') res += '\r';
                else res += esc;
            } else {
                res += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    JsonValue parse_number() {
        size_t start = pos;
        if (src[pos] == '-') pos++;
        while (pos < src.size() && (std::isdigit(src[pos]) || src[pos] == '.')) pos++;
        std::string sub = src.substr(start, pos - start);
        JsonValue v;
        v.type = JsonType::Number;
        v.num_val = std::stod(sub);
        return v;
    }

public:
    explicit JsonParser(std::string text) : src(std::move(text)) {}

    JsonValue parse_value() {
        skip_whitespace();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') {
            JsonValue v;
            v.type = JsonType::String;
            v.str_val = parse_string();
            return v;
        }
        if (std::isdigit(c) || c == '-') return parse_number();
        if (src.compare(pos, 4, "true") == 0) { pos += 4; JsonValue v; v.type = JsonType::Boolean; v.bool_val = true; return v; }
        if (src.compare(pos, 5, "false") == 0) { pos += 5; JsonValue v; v.type = JsonType::Boolean; v.bool_val = false; return v; }
        if (src.compare(pos, 4, "null") == 0) { pos += 4; JsonValue v; v.type = JsonType::Null; return v; }
        throw std::runtime_error(std::string("Unexpected token near pos ") + std::to_string(pos));
    }

    JsonValue parse_array() {
        get(); // '['
        JsonValue val;
        val.type = JsonType::Array;
        if (peek() == ']') { get(); return val; }
        while (true) {
            val.arr_val.push_back(parse_value());
            char next = peek();
            if (next == ']') { get(); break; }
            if (next == ',') { get(); continue; }
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        return val;
    }

    JsonValue parse_object() {
        get(); // '{'
        JsonValue val;
        val.type = JsonType::Object;
        if (peek() == '}') { get(); return val; }
        while (true) {
            if (peek() != '"') throw std::runtime_error("Expected key string in object");
            std::string key = parse_string();
            if (get() != ':') throw std::runtime_error("Expected ':' after key");
            val.obj_val[key] = parse_value();
            char next = peek();
            if (next == '}') { get(); break; }
            if (next == ',') { get(); continue; }
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        return val;
    }
};
