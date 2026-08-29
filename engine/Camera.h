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

	float yaw = -90.0f; //sağ-sol
	float pitch = 0.0f; //yukarı-aşağı
	float moveSpeed = 250.0f;
	float mouseSensitivity = 0.1f;
	float verticalVelocity = 0.0f;

	bool onGround = false;
	bool isCrouching = false;

	float bobTimer = 0.0f;
	float bobOffsetY = 0.0f;
	float bobOffsetX = 0.0f;

	int viewBobStyle = 1;       //sinus
	float doomBobPhase = 0.0f;

	glm::vec3 GetEyePosition() const {
		glm::vec3 base = position + glm::vec3(0.0f, eyeHeightOffset + bobOffsetY, 0.0f);
		return base + Right() * bobOffsetX;
	}

	glm::vec3 Forward() const; //Kameranın baktığı yön
	glm::vec3 FlatForward() const;
	glm::vec3 Right() const;
	glm::vec3 Up() const;

	void MoveForward(float deltaTime);
	void MoveBackward(float deltaTime);
	void MoveRight(float deltaTime);
	void MoveLeft(float deltaTime);

	void ProcessMouseMovement(float xOffset, float yOffset);
	void Update(float deltaTime); // her frame çağrılacak
	void UpdateViewBob(float deltaTime, float horizontalSpeed, bool grounded);

	glm::mat4 GetViewMatrix() const;
	float roll = 0.0f;
	float targetRoll = 0.0f;
	float maxRoll = 7.0f;        // derece cinsinden max eğim
	float rollLerpSpeed = 8.0f; // ne kadar hızlı düzelsin/eğilsin

	

};
extern Camera g_Camera;