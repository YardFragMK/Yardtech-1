#include <iostream>
#include"engine/Engine.h"
int main(int argv, char* arvc[])
{
    Engine engine;
   // engine.initSystems();
    if (!engine.initSystems()) {
        return -1;
    }
    engine.gameLoop();
    return 0;
}

