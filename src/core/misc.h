#pragma once
#include <stdexcept>

#define DATABASE_THROW_EXCEPTION(msg)   do {throw std::runtime_error(std::string(__FILE__) + ":" + std::string(__func__) + "(" + std::to_string(__LINE__) + "): " + msg);}while(0)
#define NOT_IMPLEMENTED_EXCEPTION       DATABASE_THROW_EXCEPTION("NOT IMPLEMENTED YET")
