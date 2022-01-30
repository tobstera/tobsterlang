#ifndef TOBSTERLANG_UTILS_H
#define TOBSTERLANG_UTILS_H

#include <iostream>
#include <string>

namespace Tobsterlang::Utils {
inline auto unescape(std::string const& str) {
    std::string result;
    result.reserve(str.size());

    for (auto i = 0u; i < str.size(); ++i) {
        if (str[i] == '\\') {
            switch (str[++i]) {
            case '\\':
                result += '\\';
                break;
            case '\'':
                result += '\'';
                break;
            case '\"':
                result += '\"';
                break;
            case '0':
                result += '\0';
                break;
            case 'a':
                result += '\a';
                break;
            case 'b':
                result += '\b';
                break;
            case 'f':
                result += '\f';
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case 't':
                result += '\t';
                break;
            case 'v':
                result += '\v';
                break;
            default:
                std::cerr << "Unknown escape sequence: \\" << str[i]
                          << std::endl;
                break;
            }
        } else {
            result += str[i];
        }
    }

    return result;
}
}  // namespace Tobsterlang::Utils

#endif  // TOBSTERLANG_UTILS_H
