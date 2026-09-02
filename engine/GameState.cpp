#include "GameState.h"

GameState g_State = GameState::MenuStatic;

void EnterMenuStatic() {
    g_State = GameState::MenuStatic;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
}

void EnterMenuLive() {
    g_State = GameState::MenuLive;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
}

void EnterPlaying() {
    g_State = GameState::Playing;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
}

void EnterPaused() {
    g_State = GameState::Paused;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
}

void ResumeFromPause() {
    g_State = GameState::Playing;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
}