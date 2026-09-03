#pragma once
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

class Camera {
public:
	glm::vec3 position = glm::vec3(0.0f, 60.0f, 0.0f);
	float eyeHeightOffset = 30.0f;
	float eyeHeightStanding = 28.0f;
	float eyeHeightCrouching = 12.0f;
	float eyeHeightLerpSpeed = 10.0f;

	float yaw = -90.0f;
	float pitch = 0.0f;
	float moveSpeed = 320.0f; 
	float mouseSensitivity = 0.1f;
	float verticalVelocity = 0.0f;

	bool onGround = false;
	bool isCrouching = false;

	float bobTimer = 0.0f;
	float bobOffsetY = 0.0f;
	float bobOffsetX = 0.0f;

	int viewBobStyle = 1;
	float doomBobPhase = 0.0f;

	// Yere sert inis anindaki dikey kamera darbesi. Inis aninda negatif bir
	// deger atanir (kamera asagi cokermis gibi), sonra her frame sifira dogru soner.
	float landingShakeOffset = 0.0f;
	float landingShakeDecaySpeed = 9.0f;

	glm::vec3 GetEyePosition() const {
		glm::vec3 base = position + glm::vec3(0.0f, eyeHeightOffset + bobOffsetY + landingShakeOffset, 0.0f);
		return base + Right() * bobOffsetX;
	}

	glm::vec3 Forward() const;
	glm::vec3 FlatForward() const;
	glm::vec3 Right() const;
	glm::vec3 Up() const;

	void MoveForward(float deltaTime);
	void MoveBackward(float deltaTime);
	void MoveRight(float deltaTime);
	void MoveLeft(float deltaTime);

	void ProcessMouseMovement(float xOffset, float yOffset);
	void Update(float deltaTime);
	void UpdateViewBob(float deltaTime, float horizontalSpeed, bool grounded);

	// impactSpeed: iniş anindaki dikey hizin buyuklugu, darbenin siddetini olceklendirir.
	void TriggerLandingShake(float shakeAmount);

	glm::mat4 GetViewMatrix() const;
	float roll = 0.0f;
	float targetRoll = 0.0f;
	float maxRoll = 7.0f;
	float rollLerpSpeed = 8.0f;
};
extern Camera g_Camera;