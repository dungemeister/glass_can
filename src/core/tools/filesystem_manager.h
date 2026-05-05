#pragma once

#include <filesystem>
#include <string>

namespace FilesystemManager{

    static inline bool mkdir(const std::string& path){
        return std::filesystem::create_directories(path);
    }

    static inline bool cp(const std::string& src, const std::string& dst){
        return std::filesystem::copy_file(src, dst);
    }

    static inline bool rm_rf(const std::string& path){
        return std::filesystem::remove_all(path);
    }

    static inline std::string abs_path(const std::string& rel_path){
        return std::filesystem::absolute(rel_path);
    }
}; //namespace FilesystemManager