#pragma once
#include <string>

struct SurveyLink{
    std::string url;
    std::string title;
    int period;

    std::string repr() const { return url + " " + title + " " + std::to_string(period); }
};