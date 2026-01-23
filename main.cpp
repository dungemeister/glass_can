#include <iostream>
#include <unistd.h>

#include "bot.h"

int main(int argc, char* argv[]){
    std::cout << "Hello from telegram bot. PID: " << getpid() << std::endl;
    std::filesystem::path currentDir;
     try {
        currentDir = std::filesystem::current_path();

    } catch (std::filesystem::filesystem_error const& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        exit(-1);
    }

    std::string config_file = currentDir.string() + "/config.json";

    TgBot bot(config_file);

    bot.loop();
}