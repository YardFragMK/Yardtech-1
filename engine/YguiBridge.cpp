#include "YguiBridge.h"
#include "InputBridge.h"
#include "yg_ygui.h"

namespace {
    // SDL dugme numarasini ImGui duzenine cevirir (0 sol, 1 sag, 2 orta); -1: desteklenmiyor.
    int ToUiMouseButton(Uint8 sdlButton) {
        switch (sdlButton) {
        case SDL_BUTTON_LEFT:   return 0;
        case SDL_BUTTON_RIGHT:  return 1;
        case SDL_BUTTON_MIDDLE: return 2;
        default:                return -1;
        }
    }

    // SDL_TEXTINPUT'taki UTF-8 diziyi codepoint'lere cevirip tek tek iletir.
    void ForwardUtf8Text(const char* text) {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(text);
        while (*p != 0) {
            unsigned int codepoint = 0;
            int extra = 0;

            if (*p < 0x80) { codepoint = *p;        extra = 0; }
            else if ((*p & 0xE0) == 0xC0) { codepoint = *p & 0x1F; extra = 1; }
            else if ((*p & 0xF0) == 0xE0) { codepoint = *p & 0x0F; extra = 2; }
            else if ((*p & 0xF8) == 0xF0) { codepoint = *p & 0x07; extra = 3; }
            else { ++p; continue; }

            ++p;
            bool valid = true;
            for (int i = 0; i < extra; i++) {
                if ((*p & 0xC0) != 0x80) { valid = false; break; }
                codepoint = (codepoint << 6) | (*p & 0x3F);
                ++p;
            }
            if (valid) YGUI::AddInputCharacter(codepoint);
        }
    }
}

void ForwardEventToYgui(const SDL_Event& event, bool consoleOpen) {
    switch (event.type) {
    case SDL_MOUSEMOTION:
        if (!consoleOpen) {
            YGUI::AddMousePosEvent(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
        }
        break;
    case SDL_MOUSEBUTTONDOWN: {
        const int button = ToUiMouseButton(event.button.button);
        if (!consoleOpen && button >= 0) YGUI::AddMouseButtonEvent(button, true);
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        const int button = ToUiMouseButton(event.button.button);
        if (button >= 0) YGUI::AddMouseButtonEvent(button, false);
        break;
    }
    case SDL_MOUSEWHEEL:
        if (!consoleOpen) {
            YGUI::AddMouseWheelEvent(static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y));
        }
        break;
    case SDL_TEXTINPUT:
        if (!consoleOpen) ForwardUtf8Text(event.text.text);
        break;
    case SDL_KEYDOWN: {
        if (consoleOpen || event.key.repeat) break; // tekrari ImGui kendisi uretir
        const int code = SDLScancodeToButtonCode(event.key.keysym.scancode);
        if (code != 0) YGUI::AddKeyEvent(code, true);
        break;
    }
    case SDL_KEYUP: {
        const int code = SDLScancodeToButtonCode(event.key.keysym.scancode);
        if (code != 0) YGUI::AddKeyEvent(code, false);
        break;
    }
    default:
        break;
    }
}