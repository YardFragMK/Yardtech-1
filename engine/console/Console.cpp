#include"Console.h"
#include"CVar.h"
#include<sstream>
#include<sstream>
#include<iostream>

bool Console::s_open = false;
std::string Console::s_input;

void Console::Init() {
    std::cout << "Yardtech-1 version: alpha 0.1\n";
    std::cout << "=============================\n\n";
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
        std::cout << s_input << std::endl;
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
        g_CVar.nvs_cheatsF();
    }
    else if (cmd == "cm_noclip") {
        g_CVar.cm_noclipF();
    }
    else if (cmd == "cm_speed") {
        float value;
        if (iss >> value) g_CVar.cm_speedF(value);
        else std::cout << "Usage: cm_speed <value>\n";
    }
    else if (cmd == "cm_sensitivity") {
        float value;
        if (iss >> value) g_CVar.cm_sensitivityF(value);
    }
    else if (cmd == "cm_rollmax") {
        float value;
        if (iss >> value) g_CVar.cm_rollmaxF(value);
    }
    else if (cmd == "cm_rollspeed") {
        float value;
        if (iss >> value) g_CVar.cm_rollspeedF(value);
    }
    else if (cmd == "nvs_developer") {
        int value;
        if (iss >> value) g_CVar.nvs_developerF(value);
        else std::cout << "Usage: nvs_developer <0/1>\n";
    }
    else {
        std::cout << "Unknown command\n";
    }
}



