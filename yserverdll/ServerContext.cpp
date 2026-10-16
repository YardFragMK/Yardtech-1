#include "pch.h"
#include "ServerContext.h"
#include <cfloat>

ServerContext& Ctx() {
    static ServerContext context;
    return context;
}

void ServerContext::Log(const char* message) const {
    if (console != nullptr) console->Print(message);
}

PlayerSlot* ServerContext::GetSlot(int clientIndex) {
    if (clientIndex < 0 || clientIndex >= MAX_PLAYERS) return nullptr;
    return &slots[clientIndex];
}

PlayerSlot* ServerContext::FindNearestPlayer(const glm::vec3& from) {
    PlayerSlot* best = nullptr;
    float bestDistance = FLT_MAX;

    for (PlayerSlot& slot : slots) {
        if (!slot.connected) continue;

        float distance = glm::length(slot.eyePosition - from);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = &slot;
        }
    }
    return best;
}