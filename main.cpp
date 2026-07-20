#include <iostream>
#include"engine/Engine.h"

int main(int argc, char* argv[]){

    Engine engine;
    if (!engine.initSystems()) {
        return -1;
    }
    engine.gameLoop();
    return 0;
}

