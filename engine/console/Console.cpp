#include"Console.h"
#include"CVar.h"
#include<sstream>
#include<iostream>
#include"../Camera.h"

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
    else if (cmd == "help") {
        std::cout << "nvs_cheats " << g_CVar.nvs_cheats << std::endl;
        std::cout << "cm_noclip " << g_CVar.cm_noclip << std::endl;
        std::cout << "cm_sensitivity " << g_Camera.mouseSensitivity << std::endl;
        std::cout << "nvs_developer " << g_CVar.nvs_developer << std::endl;
        if (g_CVar.nvs_developer == 1) {
            std::cout << "cm_speed " << g_Camera.moveSpeed << std::endl;
            std::cout << "cm_rollmax " << g_Camera.maxRoll << std::endl;
            std::cout << "cm_rollspeed " << g_Camera.rollLerpSpeed << std::endl;
        }
    }
    else {
        std::cout << "Unknown command\n";
    }
}



