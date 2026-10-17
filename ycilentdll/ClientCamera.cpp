#include "pch.h"
#include "ClientCamera.h"
#include <cmath>

glm::vec3 ClientCamera::Forward() const {
    ViewSetup view;
    view.yaw = yaw;
    view.pitch = pitch;
    return ViewForward(view);
}

glm::vec3 ClientCamera::Right() const {
    return glm::normalize(glm::cross(Forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 ClientCamera::GetEyePosition() const {
    glm::vec3 base = position + glm::vec3(0.0f, eyeHeightOffset + bobOffsetY + landingShakeOffset, 0.0f);
    return base + Right() * bobOffsetX;
}

void ClientCamera::Reset(const glm::vec3& newPosition, float newYaw) {
    position = newPosition;
    yaw = newYaw;
    isCrouching = false;
}

void ClientCamera::ProcessMouseMovement(float xOffset, float yOffset, float sensitivity) {
    yaw += xOffset * sensitivity;
    pitch -= yOffset * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void ClientCamera::SetRollInput(float moveRightAxis, float maxRoll) {
    if (moveRightAxis > 0.0f) {
        targetRoll = maxRoll;
    }
    else if (moveRightAxis < 0.0f) {
        targetRoll = -maxRoll;
    }
}

void ClientCamera::Update(float deltaTime, float rollLerpSpeed) {
    roll = glm::mix(roll, targetRoll, glm::clamp(rollLerpSpeed * deltaTime, 0.0f, 1.0f));
    targetRoll = 0.0f;

    float targetEyeHeight = isCrouching ? eyeHeightCrouching : eyeHeightStanding;
    eyeHeightOffset = glm::mix(eyeHeightOffset, targetEyeHeight,
        glm::clamp(eyeHeightLerpSpeed * deltaTime, 0.0f, 1.0f));

    // Inis darbesi zamanla sifira soner, kamera normal yuksekligine doner.
    landingShakeOffset = glm::mix(landingShakeOffset, 0.0f,
        glm::clamp(landingShakeDecaySpeed * deltaTime, 0.0f, 1.0f));
}

void ClientCamera::TriggerLandingShake(float shakeAmount) {
    landingShakeOffset = -shakeAmount;
}

void ClientCamera::UpdateViewBob(float deltaTime, float horizontalSpeed, bool grounded, int style) {
    if (style == 1) {
        const float DOOM_BOB_AMP = 7.0f;
        const float DOOM_BOB_FREQ = 0.045f;

        float distanceThisFrame = horizontalSpeed * deltaTime;
        float intensity = (grounded && horizontalSpeed > 1.0f) ? 1.0f : 0.0f;

        doomBobPhase += distanceThisFrame * DOOM_BOB_FREQ;
        bobOffsetY = sinf(doomBobPhase) * DOOM_BOB_AMP * intensity;
        bobOffsetX = 0.0f;
        return;
    }

    const float BOB_FREQUENCY = 8.0f;
    const float BOB_AMP_Y = 1.6f;
    const float BOB_AMP_X = 1.0f;
    const float SPEED_REF = 150.0f;

    float intensity = grounded ? (horizontalSpeed / SPEED_REF) : 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    if (intensity > 0.01f) {
        bobTimer += deltaTime * BOB_FREQUENCY * intensity;
    }
    else {
        bobTimer += deltaTime * BOB_FREQUENCY * 0.15f;
    }

    bobOffsetY = sinf(bobTimer * 2.0f) * BOB_AMP_Y * intensity;
    bobOffsetX = cosf(bobTimer) * BOB_AMP_X * intensity;
}

void ClientCamera::FillView(ViewSetup* outView) const {
    if (outView == nullptr) return;

    outView->origin = GetEyePosition();
    outView->yaw = yaw;
    outView->pitch = pitch;
    outView->roll = roll;
}