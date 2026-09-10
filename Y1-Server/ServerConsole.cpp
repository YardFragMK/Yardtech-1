#include "ServerConsole.h"
#include <thread>
#include <mutex>
#include <deque>
#include <iostream>
#include <atomic>

namespace {
    std::thread s_thread;
    std::mutex s_mutex;
    std::deque<std::string> s_commandQueue;
    std::atomic<bool> s_running{ false };

    void ReadLoop() {
        std::string line;
        while (s_running && std::getline(std::cin, line)) {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_commandQueue.push_back(line);
        }
    }
}

void ServerConsole::Start() {
    s_running = true;
    s_thread = std::thread(ReadLoop);
}

void ServerConsole::Stop() {
    s_running = false;
    // std::cin okumasi bloklu oldugu icin thread'i "zorla" durdurmuyoruz;
    // program kapanirken isletim sistemi kaynagi geri alir. detach ile
    // ana thread'in cikisini beklemesine gerek birakmiyoruz.
    if (s_thread.joinable()) {
        s_thread.detach();
    }
}

bool ServerConsole::PopCommand(std::string& outLine) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_commandQueue.empty()) return false;
    outLine = s_commandQueue.front();
    s_commandQueue.pop_front();
    return true;
}