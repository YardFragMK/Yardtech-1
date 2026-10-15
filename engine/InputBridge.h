#pragma once
#include <SDL.h>

// SDL olaylarini engine-bagimsiz ButtonCode degerlerine ve IBaseClientDLL
// cagrilarina cevirir. client.dll SDL'i hic bilmez.
void ForwardEventToClient(const SDL_Event& event, bool consoleOpen);

// Verilen ButtonCode'un su an basili olup olmadigini dondurur.
bool IsButtonCodeDown(int buttonCode);

int SDLScancodeToButtonCode(SDL_Scancode scancode);