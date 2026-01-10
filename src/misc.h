#pragma once
#include <string>

namespace StringMisc{
    static std::string escapeString(const std::string& input, const std::string& chars=".{}-") {
        std::string result;
        result.reserve(input.size() * 2);  // Резервируем память для оптимизации
        
        for (char c : input) {
            if (chars.find(c) != std::string::npos) {
                result += '\\';
            }
            result += c;
        }
        
        return result;
    }

}