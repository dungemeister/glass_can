#pragma once
#include <string>
#include <iostream>
#include "nlohmann/json.hpp"
#include <regex>

namespace StringMisc{
    const std::string c_full_escaping_sequence = "_~><#+-_=().!|";

    enum SplitOnce{
        FROM_START,
        FROM_END,
    };

    using json = nlohmann::json;
    static std::string escapeString(const std::string& input, const std::string& chars="_~>#+-=|{}.!") {
        std::string result;
        result.reserve(input.size() * 2);
        
        for (char c : input) {
            if (chars.find(c) != std::string::npos) {
                result += R"(\)";
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

    static std::string getUriNameFromSteamLink(const std::string& url){
        auto index = url.rfind("/");
        return  url.substr(index + 1);
    }

    static std::string createMarkdownLink(const std::string& url, const std::string& text){
        std::stringstream out;
        auto text_copy = removeQuotes(text);
        // text_copy = escapeString(text_copy, "_~>#+-=|{}.!()");
        
        auto url_copy  = removeQuotes(url);
        url_copy = escapeString(url_copy, "()");

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
    static std::string sqlQuoteShielding(const std::string& input){
        std::string result;
        result.reserve(input.size() * 2); 
        
        for (char c : input) {
            if (c == '\'') {
                result += '\'';
            }
            result += c;
        }
        
        return result;
    }

    static std::vector<std::string> splitByDelim(const std::string& input, char delim){
        std::vector<std::string> tokens;
        std::string token;
        std::stringstream sstream(input);

        while(std::getline(sstream, token, delim)){
            tokens.push_back(std::move(token));
        }

        return tokens;
    }

    static std::vector<std::string> splitByRegex(const std::string& input, const std::regex& delim_pattern){

        std::sregex_token_iterator iter(input.begin(), input.end(), delim_pattern, -1);
        std::sregex_token_iterator end;

        std::vector<std::string> res(iter, end);
        return res;
    }

    static std::vector<std::string> splitToGroupsByRegex(const std::string& input, const std::regex& delim_pattern){

        std::vector<std::string> res;
        std::smatch matches;

        if (std::regex_search(input, matches, delim_pattern)) {
        for (size_t i = 1; i < matches.size(); ++i) {
            res.emplace_back(matches[i]);
        }
        } else {
            return {};
        }
        return res;
    }
    static std::vector<std::string> splitOnceByDelim(const std::string& input, char delim, SplitOnce start=SplitOnce::FROM_START){
        std::vector<std::string> tokens;
        size_t pos;
        if(start == SplitOnce::FROM_START) pos = input.find(delim);
        else if(start == SplitOnce::FROM_END) pos = input.rfind(delim);
        else return {};
        
        if(pos == std::string::npos) return {};

        tokens.push_back(input.substr(0, pos));
        tokens.push_back(input.substr(pos+1));
        return tokens;
    }
}