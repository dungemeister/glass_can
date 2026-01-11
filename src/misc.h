#pragma once
#include <string>

namespace StringMisc{

    static std::string escapeString(const std::string& input, const std::string& chars="_~>#+-=|{}.!") {
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

    std::string getUriNameFromSteamLink(const std::string& url){
        auto index = url.rfind("/");
        return  url.substr(index + 1);
    }

    static std::string createMarkdownLink(const std::string& url, const std::string& text){
        std::stringstream out;
        auto text_copy = removeQuotes(text);
        auto url_copy  = removeQuotes(url);

        out << "[" << text_copy << "](" << url_copy << ")";

        return out.str();
    }

    static std::string createMarkdownLinkTable(const std::vector<json>& links){
        std::stringstream out;
        out << "```text\n" << std::endl;

        out << "\n|№|Ссылка|Наименование|\n" <<
               "|:--:|:------:|:------------|\n";
        try{
            size_t index = 1;
            for(auto& link: links){
                out << "|" << index << "|" <<
                       createMarkdownLink(link["url"], link["title"]) << "|" <<
                       uriToString(getUriNameFromSteamLink(link["url"])) << "|" << std::endl;
                index++;
            }
            out << "```" << std::endl;
            std::cout << out.str();
            return out.str();
        }
        catch(const std::exception& e){
            std::cerr << "createMarkdownLinkTable: " << e.what() << std::endl;
            return {};
        }
    }

    static std::string createHandmadeTable(const std::vector<json>& links){
        std::stringstream out;
        out << "```text\n" << std::endl;

        try{
            size_t index = 1;
            for(auto& link: links){
                out << index << " - " <<
                       createMarkdownLink(link["url"], link["title"]) << " - " <<
                       uriToString(getUriNameFromSteamLink(link["url"])) << std::endl;
                index++;
            }
            out << "```" << std::endl;
            std::cout << out.str();
            return out.str();
        }
        catch(const std::exception& e){
            std::cerr << "createMarkdownLinkTable: " << e.what() << std::endl;
            return {};
        }
    }
}