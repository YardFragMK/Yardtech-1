#pragma once
#include <SDL.h>

// SDL olaylarini YGUI'nin giris fonksiyonlarina cevirir. Konsol aciksken
// bu olaylar menulere iletilmez; yalnizca birakma olaylari her zaman iletilir
// ki fare dugmesi veya tus "takili" kalmasin.
void ForwardEventToYgui(const SDL_Event& event, bool consoleOpen);