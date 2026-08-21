#include"Console.h"
#include"CVar.h"
#include<sstream>
#include<iostream>
#include <algorithm>
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include"../Camera.h"
#include"../BSPReader.h"
#include"../MapLoader.h"

#define STB_EASY_FONT_IMPLEMENTATION
#include "../../extern/stb/stb_easy_font.h"

bool Console::s_open = false;
std::string Console::s_input;
std::vector<std::string> Console::s_log;
float Console::s_currentHeight = 0.0f;
float Console::s_targetHeight = 0.0f;
float Console::s_slideSpeed = 2000.0f;

void Console::Init() {
    Log("Yardtech-1 version: alpha 0.30");
    Log("=============================");
}

void Console::Toggle() {
	s_open = !s_open;
	if (s_open) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_StartTextInput();
		Logger::info("[Console opened]");
	}
	else {
        SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_StopTextInput();
		Logger::info("[Console closed]");
	}
}

void Console::Shutdown() {

}  

bool Console::IsOpen() {
	return s_open;
}

void Console::HandleEvent(const SDL_Event& event) {
    if (!s_open) {
        return;
    }

    if (event.type == SDL_TEXTINPUT) {
        s_input += event.text.text;
    }

    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_BACKSPACE:
            if (!s_input.empty()) {
                s_input.pop_back();
            }
            std::cout << s_input << std::endl;
            break;

        case SDL_SCANCODE_RETURN:
            Log("] " + s_input);
            ExecuteCommand();
            s_input.clear();
            break;
        }
    }
}

void Console::ExecuteCommand() {
    std::istringstream iss(s_input);
    std::string cmd;
    iss >> cmd;

    if (cmd == "nvs_cheats") {
        int value;
        if (iss >> value) { 
            if(value == 1 || value == 0) g_CVar.nvs_cheatsF(value);
            else Log("Usage: nvs_cheats <0/1>");
        }
        else Log("Usage: nvs_cheats <0/1>");
    }

    else if (cmd == "cm_noclip") {
        int value;
        if (iss >> value) { 
            if (value == 1 || value == 0) g_CVar.cm_noclipF(value);
            else Log("Usage: cm_noclip <0/1>");
        }
        else Log("Usage: cm_noclip <0/1>");
    }

    else if (cmd == "cm_speed") {
        float value;
        if (iss >> value) g_CVar.cm_speedF(value);
        else Log("Usage: cm_speed <value>");
    }

    else if (cmd == "cm_sensitivity") {
        float value;
        if (iss >> value) g_CVar.cm_sensitivityF(value);
        else Log("Usage: cm_sensitivity <value>");
    }

    else if (cmd == "cm_rollmax") {
        float value;
        if (iss >> value) g_CVar.cm_rollmaxF(value);
        else Log("Usage: cm_rollmax <value>");
    }

    else if (cmd == "cm_rollspeed") {
        float value;
        if (iss >> value) g_CVar.cm_rollspeedF(value);
        else Log("Usage: cm_rollspeed <value>");
    }

    else if (cmd == "nvs_developer") {
        int value;
        if (iss >> value) {
            if (value == 1 || value == 0) g_CVar.nvs_developerF(value);
            else Log("Usage: nvs_developer <0/1>");
        }
        else Log("Usage: nvs_developer <0/1>");
    }

    else if (cmd == "help") {
        g_CVar.helpF();
    }

    else if (cmd == "map") {
        std::string value;
        if (iss >> value) { 
            ReadEntityLump(value);
            LoadMap(value);
        }
        else Log("Usage: map <path>");
        
    }
    
    else if (cmd == "r_retromode") {
        int value;
        if (iss >> value) {
            if (value == 1 || value == 0) g_CVar.r_retromode(value);
            else Log("Usage: r_retromode <0/1>");
        }
        else Log("Usage: r_retromode <0/1>");
    }

    else if (cmd == "nvs_gravity") {
        int value;
        if (iss >> value) {
            g_CVar.nvs_gravityF(value);
        }
    }

    else if (cmd == "nvs_jumpforce") {
        int value;
        if (iss >> value) {
            g_CVar.nvs_jumpforceF(value);
        }
    }

    else {
        Log("Unknown command");
    }

}

void Console::Log(const std::string& line) {
    s_log.push_back(line);
    // Cok uzun gecmisi sinirla (bellek/performans icin)
    if (s_log.size() > 200) {
        s_log.erase(s_log.begin());
    }
}

// Ekranin yaklasik %40'ini kaplasin (windowHeight disaridan geldigi icin
// Update'te henuz bilinmiyor olabilir; Render icinde gercek hedefi hesapliyoruz)
void Console::Update(float deltaTime) {
    // s_targetHeight, Render() icinde windowHeight'e gore ayarlaniyor;
    // burada sadece mevcut yuksekligi hedefe dogru kaydiriyoruz.
    float diff = s_targetHeight - s_currentHeight;
    float step = s_slideSpeed * deltaTime;

    if (std::abs(diff) <= step) {
        s_currentHeight = s_targetHeight;
    }
    else {
        s_currentHeight += (diff > 0 ? step : -step);
    }
}



// stb_easy_font ile bir metin satiri cizer (basit, sabit genislikli bitmap font)
static void DrawText(float x, float y, const std::string& text, float r, float g, float b) {
    static char buffer[99999];
    unsigned char color[4] = {
        (unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), 255
    };

    int numQuads = stb_easy_font_print(x, y, (char*)text.c_str(), color, buffer, sizeof(buffer));
    if (numQuads <= 0) return;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // stb_easy_font vertex yapisi: [float x,y,z (12 byte)][unsigned char color[4] (4 byte)] = 16 byte stride
    glVertexPointer(3, GL_FLOAT, 16, buffer);
    glColorPointer(4, GL_UNSIGNED_BYTE, 16, buffer + 12);

    glDrawArrays(GL_QUADS, 0, numQuads * 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

void Console::Render(int windowWidth, int windowHeight) {
    // Konsol acikken hedef yukseklik ekranin %40'i, kapaliyken 0
    s_targetHeight = s_open ? (windowHeight * 0.4f) : 0.0f;

    if (s_currentHeight <= 0.5f && !s_open) {
        return; // tamamen kapali, cizecek bir sey yok
    }

    // 2D ortografik moda gec
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1); // (0,0) sol-ust kose

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float h = s_currentHeight;

    // Arka plan kutusu
    glColor4f(0.08f, 0.01f, 0.01f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f((float)windowWidth, 0.0f);
    glVertex2f((float)windowWidth, h);
    glVertex2f(0.0f, h);
    glEnd();

    // Alt kenarda ince bir cizgi (ayrac)
    glColor4f(0.5f, 0.4f, 0.4f, 1.0f);
    glBegin(GL_LINES);
    glVertex2f(0.0f, h);
    glVertex2f((float)windowWidth, h);
    glEnd();

    // Metin (sadece yeterince acikken ciz, kayma sirasinda tasma olmasin)
    if (h > 20.0f) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Komut gecmisi: alttan yukariya dogru, en son satir input satirinin hemen ustunde
        float lineHeight = 14.0f;
        float y = h - lineHeight * 2.5f; // input satiri icin yer birak

        int startIdx = std::max(0, (int)s_log.size() - 30); // son 30 satiri goster
        std::vector<std::string> visible(s_log.begin() + startIdx, s_log.end());

        for (auto it = visible.rbegin(); it != visible.rend(); ++it) {
            if (y < 5.0f) break; // kutunun disina tasma
            DrawText(8.0f, y, *it, 0.85f, 0.85f, 0.85f);
            y -= lineHeight;
        }

        // Input satiri (en altta, yanip sonen imlecle)
        std::string inputLine = "> " + s_input;
        bool showCursor = ((int)(SDL_GetTicks() / 400) % 2) == 0;
        if (showCursor) inputLine += "_";

        DrawText(8.0f, h - lineHeight - 6.0f, inputLine, 1.0f, 1.0f, 0.3f);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}



