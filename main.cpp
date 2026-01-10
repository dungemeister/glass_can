#include <iostream>
#include <unistd.h>

#include "bot.h"

int main(int argc, char* argv[]){
    std::cout << "Hello from telegram bot. PID: " << getpid() << std::endl;
    
    TgBot bot("8109080221:AAGk8RTXkouAgvTpPAcpQGyCxuiyeSI1t14");
    // TgBot bot("8109080221:AAGk8RTXkouAgvTpPAcpQGyCxuiyeSI1t15");

    bot.loop();
}