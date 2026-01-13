#include <iostream>
#include <unistd.h>

#include "bot.h"

int main(int argc, char* argv[]){
    std::cout << "Hello from telegram bot. PID: " << getpid() << std::endl;
    
    std::string api_filepath = "api.key";
    std::string api_key;
    std::fstream api_key_file(api_filepath);
    if(!api_key_file.is_open()){
        std::cerr <<"fail to open " << api_filepath << std::endl;
        exit(-1);
    }
    std::getline(api_key_file, api_key);

    TgBot bot(api_key);

    bot.loop();
}