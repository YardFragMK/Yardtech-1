#include <iostream>
#include"engine/Engine.h"

int main(int argc, char* argv[]){

    Engine engine;
    if (!engine.initSystems()) {
        std::cout << "Engine baslatilamadi! Bir tusa basarak kapatin..." << std::endl;
        std::cin.get();
        return -1;
    }
    engine.gameLoop();
    return 0;
}

