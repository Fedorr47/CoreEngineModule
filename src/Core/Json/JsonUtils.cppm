module;

#include <cstdlib>

export module core:json_utils;

import std;
import :math_utils;
import :string_utils;

export namespace jsonUtils
{
    struct JsonValue;
    using JsonObject = std::unordered_map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    struct JsonValue
    {
        using Storage = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
        Storage value_{};

        bool IsNull() const noexcept { return std::holds_alternative<std::nullptr_t>(value_); }
        bool IsBool() const noexcept { return std::holds_alternative<bool>(value_); }
        bool IsNumber() const noexcept { return std::holds_alternative<double>(value_); }
        bool IsString() const noexcept { return std::holds_alternative<std::string>(value_); }
        bool IsArray() const noexcept { return std::holds_alternative<JsonArray>(value_); }
        bool IsObject() const noexcept { return std::holds_alternative<JsonObject>(value_); }

        const JsonObject& AsObject() const
        {
            if (!IsObject()) throw std::runtime_error("JSON: expected object");
            return std::get<JsonObject>(value_);
        }

        const JsonArray& AsArray() const
        {
            if (!IsArray()) throw std::runtime_error("JSON: expected array");
            return std::get<JsonArray>(value_);
        }

        const std::string& AsString() const
        {
            if (!IsString()) throw std::runtime_error("JSON: expected string");
            return std::get<std::string>(value_);
        }

        double AsNumber() const
        {
            if (!IsNumber()) throw std::runtime_error("JSON: expected number");
            return std::get<double>(value_);
        }

        bool AsBool() const
        {
            if (!IsBool()) throw std::runtime_error("JSON: expected bool");
            return std::get<bool>(value_);
        }
    };

    class JsonParser
    {
    public:
        explicit JsonParser(std::string_view text) : text_(text) {}

        [[nodiscard]] JsonValue Parse()
        {
            SkipWs();
            JsonValue out = ParseValue();
            SkipWs();
            if (pos_ != text_.size())
            {
                Throw("unexpected trailing characters");
            }
            return out;
        }

    private:
        std::string_view text_;
        std::size_t pos_{};

        [[noreturn]] void Throw(std::string_view msg) const
        {
            throw std::runtime_error(std::string("JSON parse error at ") + std::to_string(pos_) + ": " + std::string(msg));
        }

        [[nodiscard]] char Peek() const noexcept
        {
            return (pos_ < text_.size()) ? text_[pos_] : '\0';
        }

        char Get()
        {
            if (pos_ >= text_.size())
            {
                Throw("unexpected end of input");
            }
            return text_[pos_++];
        }

        void SkipWs() noexcept
        {
            while (pos_ < text_.size())
            {
                const unsigned char c = static_cast<unsigned char>(text_[pos_]);
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                {
                    ++pos_;
                    continue;
                }
                break;
            }
        }

        void Expect(char expected)
        {
            SkipWs();
            const char c = Get();
            if (c != expected)
            {
                Throw(std::string("expected '") + expected + "'");
            }
        }

        [[nodiscard]] bool Match(std::string_view token)
        {
            SkipWs();
            if (text_.substr(pos_, token.size()) == token)
            {
                pos_ += token.size();
                return true;
            }
            return false;
        }

        [[nodiscard]] JsonValue ParseValue()
        {
            SkipWs();
            switch (Peek())
            {
            case '{': return ParseObject();
            case '[': return ParseArray();
            case '"':
            {
                JsonValue v; v.value_ = ParseString(); return v;
            }
            case 't':
                if (Match("true")) { JsonValue v; v.value_ = true; return v; }
                break;
            case 'f':
                if (Match("false")) { JsonValue v; v.value_ = false; return v; }
                break;
            case 'n':
                if (Match("null")) { JsonValue v; v.value_ = nullptr; return v; }
                break;
            default:
                break;
            }

            if (Peek() == '-' || (Peek() >= '0' && Peek() <= '9'))
            {
                JsonValue v; v.value_ = ParseNumber(); return v;
            }

            Throw("unexpected token");
        }

        [[nodiscard]] JsonValue ParseObject()
        {
            Expect('{');
            JsonObject obj;
            SkipWs();
            if (Peek() == '}')
            {
                Get();
                JsonValue v; v.value_ = std::move(obj); return v;
            }

            while (true)
            {
                SkipWs();
                if (Peek() != '"') Throw("expected string key");
                std::string key = ParseString();
                Expect(':');
                JsonValue value = ParseValue();
                obj.emplace(std::move(key), std::move(value));
                SkipWs();
                const char c = Get();
                if (c == '}') break;
                if (c != ',') Throw("expected ',' or '}'");
            }

            JsonValue v; v.value_ = std::move(obj); return v;
        }

        [[nodiscard]] JsonValue ParseArray()
        {
            Expect('[');
            JsonArray arr;
            SkipWs();
            if (Peek() == ']')
            {
                Get();
                JsonValue v; v.value_ = std::move(arr); return v;
            }

            while (true)
            {
                JsonValue value = ParseValue();
                arr.push_back(std::move(value));
                SkipWs();
                const char c = Get();
                if (c == ']') break;
                if (c != ',') Throw("expected ',' or ']'");
            }

            JsonValue v; v.value_ = std::move(arr); return v;
        }

        [[nodiscard]] std::string ParseString()
        {
            Expect('"');
            std::string out;
            while (true)
            {
                const char c = Get();
                if (c == '"')
                {
                    break;
                }
                if (c == '\\')
                {
                    const char e = Get();
                    switch (e)
                    {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u':
                        for (int i = 0; i < 4; ++i) { Get(); }
                        out.push_back('?');
                        break;
                    default:
                        Throw("invalid string escape");
                    }
                    continue;
                }

                out.push_back(c);
            }
            return out;
        }

        [[nodiscard]] double ParseNumber()
        {
            SkipWs();
            const std::size_t start = pos_;
            if (Peek() == '-')
            {
                ++pos_;
            }
            while (Peek() >= '0' && Peek() <= '9')
            {
                ++pos_;
            }
            if (Peek() == '.')
            {
                ++pos_;
                while (Peek() >= '0' && Peek() <= '9')
                {
                    ++pos_;
                }
            }
            if (Peek() == 'e' || Peek() == 'E')
            {
                ++pos_;
                if (Peek() == '+' || Peek() == '-') ++pos_;
                while (Peek() >= '0' && Peek() <= '9') ++pos_;
            }

            const std::string tmp(text_.substr(start, pos_ - start));
            char* endPtr = nullptr;
            const double v = std::strtod(tmp.c_str(), &endPtr);
            if (endPtr == tmp.c_str())
            {
                Throw("invalid number");
            }
            return v;
        }
    };

    [[nodiscard]] inline const JsonValue* TryGet(const JsonObject& jsonObject, std::string_view key)
    {
        if (auto it = jsonObject.find(std::string(key)); it != jsonObject.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    [[nodiscard]] inline const JsonValue& GetReq(const JsonObject& object, std::string_view key)
    {
        if (const JsonValue* value = TryGet(object, key))
        {
            return *value;
        }
        throw std::runtime_error(std::string("Level JSON: missing required field '") + std::string(key) + "'");
    }

    [[nodiscard]] inline std::string GetStringOpt(const JsonObject& object, std::string_view key, std::string def = {})
    {
        if (const JsonValue* value = TryGet(object, key))
        {
            if (value->IsString()) return value->AsString();
            throw std::runtime_error(std::string("Level JSON: expected string at '") + std::string(key) + "'");
        }
        return def;
    }

    [[nodiscard]] inline bool GetBoolOpt(const JsonObject& object, std::string_view key, bool def)
    {
        if (const JsonValue* value = TryGet(object, key))
        {
            if (value->IsBool()) return value->AsBool();
            throw std::runtime_error(std::string("Level JSON: expected bool at '") + std::string(key) + "'");
        }
        return def;
    }

    [[nodiscard]] inline float GetFloatOpt(const JsonObject& object, std::string_view key, float def)
    {
        if (const JsonValue* value = TryGet(object, key))
        {
            if (value->IsNumber()) return static_cast<float>(value->AsNumber());
            throw std::runtime_error(std::string("Level JSON: expected number at '") + std::string(key) + "'");
        }
        return def;
    }

    [[nodiscard]] inline int GetIntOpt(const JsonObject& object, std::string_view key, int def)
    {
        if (const JsonValue* value = TryGet(object, key))
        {
            if (value->IsNumber()) return static_cast<int>(value->AsNumber());
            throw std::runtime_error(std::string("Level JSON: expected number at '") + std::string(key) + "'");
        }
        return def;
    }

    [[nodiscard]] inline std::vector<float> ReadFloatArray(const JsonValue& value, std::size_t expected, std::string_view what)
    {
        if (!value.IsArray())
        {
            throw std::runtime_error(std::string("Level JSON: expected array for '") + std::string(what) + "'");
        }
        const auto& array = value.AsArray();
        if (array.size() != expected)
        {
            throw std::runtime_error(std::string("Level JSON: '") + std::string(what) + "' must have " + std::to_string(expected) + " elements");
        }
        std::vector<float> out;
        out.reserve(expected);
        for (const auto& item : array)
        {
            if (!item.IsNumber())
            {
                throw std::runtime_error(std::string("Level JSON: '") + std::string(what) + "' must contain numbers");
            }
            out.push_back(static_cast<float>(item.AsNumber()));
        }
        return out;
    }

    [[nodiscard]] inline mathUtils::Mat4 ReadMat4ColumnMajor16(const JsonValue& value, std::string_view what)
    {
        const auto data = ReadFloatArray(value, 16, what);
        mathUtils::Mat4 matrix(0.0f);
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                matrix[col][row] = data[static_cast<std::size_t>(col * 4 + row)];
            }
        }
        return matrix;
    }

    inline void WriteJsonEscaped(std::ostream& os, std::string_view text)
    {
        os << '"';
        for (char c : text)
        {
            switch (c)
            {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\b': os << "\\b"; break;
            case '\f': os << "\\f"; break;
            case '\n': os << "\\n"; break;
            case '\r': os << "\\r"; break;
            case '\t': os << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    static const char* hex = "0123456789ABCDEF";
                    os << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
                }
                else
                {
                    os << c;
                }
                break;
            }
        }
        os << '"';
    }

    inline void WriteJsonBool(std::ostream& os, bool value)
    {
        os << (value ? "true" : "false");
    }

    inline void WriteJsonFloat(std::ostream& os, float value)
    {
        os << (std::isfinite(value) ? value : 0.0f);
    }

    inline void WriteJsonVec3(std::ostream& os, const mathUtils::Vec3& value)
    {
        os << '[';
        WriteJsonFloat(os, value.x); os << ',';
        WriteJsonFloat(os, value.y); os << ',';
        WriteJsonFloat(os, value.z);
        os << ']';
    }

    inline void WriteJsonVec4(std::ostream& os, const mathUtils::Vec4& value)
    {
        os << '[';
        WriteJsonFloat(os, value.x); os << ',';
        WriteJsonFloat(os, value.y); os << ',';
        WriteJsonFloat(os, value.z); os << ',';
        WriteJsonFloat(os, value.w);
        os << ']';
    }

    inline void WriteJsonMat4ColMajor16(std::ostream& os, const mathUtils::Mat4& matrix)
    {
        os << '[';
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                if (col != 0 || row != 0)
                {
                    os << ',';
                }
                WriteJsonFloat(os, matrix[col][row]);
            }
        }
        os << ']';
    }

    template <typename TMap>
    [[nodiscard]] inline std::vector<std::string> SortedStringKeys(const TMap& map)
    {
        std::vector<std::string> keys;
        keys.reserve(map.size());
        for (const auto& [key, _] : map)
        {
            keys.push_back(key);
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }
}
