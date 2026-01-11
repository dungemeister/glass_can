#pragma once
#include <string>

namespace StringMisc{
    static std::string escapeString(const std::string& input, const std::string& chars="_*[]()~`>#+-=|{}.!") {
        std::string result;
        result.reserve(input.size() * 2);
        
        for (char c : input) {
            if (chars.find(c) != std::string::npos) {
                result += '\\';
            }
            result += c;
        }
        
        return result;
    }

    static std::string removeQuotes(const std::string& input){
        std::string result;
        result.reserve(input.size() * 2); 
        
        for (char c : input) {
            if (c != '"') {
                result += c;
            }
        }
        
        return result;
    }

    static std::string uriToString(const std::string& input){
        std::string result;
        result.reserve(input.length());

        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '%' && i + 2 < input.length()) {
                // Extract hex value %XX
                std::string hex = input.substr(i + 1, 2);
                
                try {
                    // Converting hex to char
                    unsigned char decoded = static_cast<unsigned char>(
                        std::stoul(hex, nullptr, 16)
                    );
                    result += decoded;
                    i += 2;  // Skip hex value XX
                } catch (...) {
                    // Add irreversible hex as is
                    result += input[i];
                }
            } else if (input[i] == '+') {
                result += ' ';
            } else {
                result += input[i];
            }
        }
        
        return result;
        
    }

    std::string createMarkdownLink(const std::string& url, const std::string& text){
        std::stringstream out;
        auto text_copy = removeQuotes(text);
        auto url_copy  = removeQuotes(url);

        out << "[" << text_copy << "](" << url_copy << ")" << std::endl;

        return out.str();
    }
}