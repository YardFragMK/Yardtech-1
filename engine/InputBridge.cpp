#include "InputBridge.h"
#include "buttoncodes.h"
#include "iclientdll.h"
#include "ModuleLoader.h"

int SDLScancodeToButtonCode(SDL_Scancode scancode) {
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) return 'A' + (scancode - SDL_SCANCODE_A);
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) return '1' + (scancode - SDL_SCANCODE_1);
    if (scancode == SDL_SCANCODE_0) return '0';
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) return BUTTON_F1 + (scancode - SDL_SCANCODE_F1);

    // Sag ve sol modifier tuslari ayni koda eslenir; oyun kodu ayrim yapmaz.
    switch (scancode) {
    case SDL_SCANCODE_SPACE:     return BUTTON_SPACE;
    case SDL_SCANCODE_ESCAPE:    return BUTTON_ESCAPE;
    case SDL_SCANCODE_RETURN:    return BUTTON_ENTER;
    case SDL_SCANCODE_TAB:       return BUTTON_TAB;
    case SDL_SCANCODE_BACKSPACE: return BUTTON_BACKSPACE;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:     return BUTTON_LCTRL;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:    return BUTTON_LSHIFT;
    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:      return BUTTON_LALT;
    case SDL_SCANCODE_GRAVE:     return BUTTON_GRAVE;
    case SDL_SCANCODE_UP:        return BUTTON_UP;
    case SDL_SCANCODE_DOWN:      return BUTTON_DOWN;
    case SDL_SCANCODE_LEFT:      return BUTTON_LEFT;
    case SDL_SCANCODE_RIGHT:     return BUTTON_RIGHT;
    default:                     return BUTTON_INVALID;
    }
}

static int SDLMouseButtonToButtonCode(Uint8 button) {
    switch (button) {
    case SDL_BUTTON_LEFT:   return BUTTON_MOUSE_LEFT;
    case SDL_BUTTON_RIGHT:  return BUTTON_MOUSE_RIGHT;
    case SDL_BUTTON_MIDDLE: return BUTTON_MOUSE_MIDDLE;
    default:                return BUTTON_INVALID;
    }
}

bool IsButtonCodeDown(int buttonCode) {
    if (buttonCode >= BUTTON_MOUSE_LEFT && buttonCode <= BUTTON_MOUSE_MIDDLE) {
        Uint32 mask = SDL_GetMouseState(nullptr, nullptr);
        switch (buttonCode) {
        case BUTTON_MOUSE_LEFT:   return (mask & SDL_BUTTON_LMASK) != 0;
        case BUTTON_MOUSE_RIGHT:  return (mask & SDL_BUTTON_RMASK) != 0;
        case BUTTON_MOUSE_MIDDLE: return (mask & SDL_BUTTON_MMASK) != 0;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if (buttonCode >= 'A' && buttonCode <= 'Z') return keys[SDL_SCANCODE_A + (buttonCode - 'A')] != 0;
    if (buttonCode >= '1' && buttonCode <= '9') return keys[SDL_SCANCODE_1 + (buttonCode - '1')] != 0;
    if (buttonCode == '0') return keys[SDL_SCANCODE_0] != 0;
    if (buttonCode >= BUTTON_F1 && buttonCode <= BUTTON_F12) return keys[SDL_SCANCODE_F1 + (buttonCode - BUTTON_F1)] != 0;

    switch (buttonCode) {
    case BUTTON_SPACE:     return keys[SDL_SCANCODE_SPACE] != 0;
    case BUTTON_ESCAPE:    return keys[SDL_SCANCODE_ESCAPE] != 0;
    case BUTTON_ENTER:     return keys[SDL_SCANCODE_RETURN] != 0;
    case BUTTON_TAB:       return keys[SDL_SCANCODE_TAB] != 0;
    case BUTTON_BACKSPACE: return keys[SDL_SCANCODE_BACKSPACE] != 0;
    case BUTTON_LCTRL:     return keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    case BUTTON_LSHIFT:    return keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    case BUTTON_LALT:      return keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];
    case BUTTON_GRAVE:     return keys[SDL_SCANCODE_GRAVE] != 0;
    case BUTTON_UP:        return keys[SDL_SCANCODE_UP] != 0;
    case BUTTON_DOWN:      return keys[SDL_SCANCODE_DOWN] != 0;
    case BUTTON_LEFT:      return keys[SDL_SCANCODE_LEFT] != 0;
    case BUTTON_RIGHT:     return keys[SDL_SCANCODE_RIGHT] != 0;
    default:               return false;
    }
}

// SDL_TEXTINPUT olayindaki UTF-8 diziyi codepoint'lere cevirip tek tek iletir.
static void ForwardUtf8Text(IBaseClientDLL* client, const char* text) {
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
        if (valid) client->OnTextInput(codepoint);
    }
}

void ForwardEventToClient(const SDL_Event& event, bool consoleOpen) {
    IBaseClientDLL* client = GetClientDLL();
    if (client == nullptr) return;

    switch (event.type) {
    case SDL_KEYDOWN: {
        // Konsol aciksken ve tus tekrar olaylarinda client'a iletilmez.
        if (consoleOpen || event.key.repeat) return;
        int code = SDLScancodeToButtonCode(event.key.keysym.scancode);
        if (code != BUTTON_INVALID) client->OnButtonEvent(code, true);
        break;
    }
    case SDL_KEYUP: {
        // Birakma olaylari her zaman iletilir; aksi halde tus "takili" kalir.
        int code = SDLScancodeToButtonCode(event.key.keysym.scancode);
        if (code != BUTTON_INVALID) client->OnButtonEvent(code, false);
        break;
    }
    case SDL_MOUSEBUTTONDOWN: {
        if (consoleOpen) return;
        int code = SDLMouseButtonToButtonCode(event.button.button);
        if (code != BUTTON_INVALID) client->OnButtonEvent(code, true);
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        int code = SDLMouseButtonToButtonCode(event.button.button);
        if (code != BUTTON_INVALID) client->OnButtonEvent(code, false);
        break;
    }
    case SDL_MOUSEMOTION:
        if (consoleOpen) return;
        client->OnMouseMove(static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel),
            static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
        break;
    case SDL_MOUSEWHEEL:
        if (consoleOpen) return;
        client->OnMouseWheel(static_cast<float>(event.wheel.y));
        break;
    case SDL_TEXTINPUT:
        if (consoleOpen) return;
        ForwardUtf8Text(client, event.text.text);
        break;
    default:
        break;
    }
}