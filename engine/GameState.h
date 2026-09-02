#pragma once
#include <SDL.h>

enum class GameState { MenuStatic, MenuLive, Playing, Paused };
extern GameState g_State;

void EnterMenuStatic();
void EnterMenuLive();
void EnterPlaying();
void EnterPaused();      // Oyunu durdurur, mouse'u serbest birakir, donuk dunyanin ustune pause menusu cizilir.
void ResumeFromPause();  // Playing'e geri doner, mouse tekrar kilitlenir.