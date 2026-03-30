module;

#include <cctype>
#include <string>
#include <string_view>

export module core:string_utils;

export namespace stringUtils
{
    [[nodiscard]] inline char ToLowerAscii(char c) noexcept
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    [[nodiscard]] inline std::string ToLowerAsciiCopy(std::string_view text)
    {
        std::string result(text);
        for (char& c : result)
        {
            c = ToLowerAscii(c);
        }
        return result;
    }

    [[nodiscard]] inline bool ContainsInsensitive(std::string_view text, std::string_view needle) noexcept
    {
        if (needle.empty() || needle.size() > text.size())
        {
            return false;
        }

        for (std::size_t i = 0; i + needle.size() <= text.size(); ++i)
        {
            bool match = true;
            for (std::size_t j = 0; j < needle.size(); ++j)
            {
                if (ToLowerAscii(text[i + j]) != ToLowerAscii(needle[j]))
                {
                    match = false;
                    break;
                }
            }
            if (match)
            {
                return true;
            }
        }
        return false;
    }
}
