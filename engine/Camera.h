#pragma once
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>

class Camera {
public:
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	float yaw = -90.0f; //sağ-sol
	float pitch = 0.0f; //yukarı-aşağı
	float moveSpeed = 100.0f;
	float mouseSensitivity = 0.1f;

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

	glm::mat4 GetViewMatrix() const;
	float roll = 0.0f;
	float targetRoll = 0.0f;
	float maxRoll = 10.0f;        // derece cinsinden max eğim
	float rollLerpSpeed = 8.0f; // ne kadar hızlı düzelsin/eğilsin

	

};
extern Camera g_Camera;