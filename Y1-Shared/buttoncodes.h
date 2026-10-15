#pragma once

// Engine'den bagimsiz tus kodlari. Exe, SDL scancode'larini bu kodlara
// cevirir; client.dll SDL'i hic bilmez. Harfler ve rakamlar buyuk harf
// ASCII degerleriyle ('A'..'Z', '0'..'9') temsil edilir.
enum ButtonCode : int {
    BUTTON_INVALID = 0,
    BUTTON_SPACE = 32,

    BUTTON_ESCAPE = 256, BUTTON_ENTER, BUTTON_TAB, BUTTON_BACKSPACE,
    BUTTON_LCTRL, BUTTON_LSHIFT, BUTTON_LALT, BUTTON_GRAVE,
    BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT,
    BUTTON_F1, BUTTON_F2, BUTTON_F3, BUTTON_F4, BUTTON_F5, BUTTON_F6,
    BUTTON_F7, BUTTON_F8, BUTTON_F9, BUTTON_F10, BUTTON_F11, BUTTON_F12,

    BUTTON_MOUSE_LEFT = 512, BUTTON_MOUSE_RIGHT, BUTTON_MOUSE_MIDDLE
};