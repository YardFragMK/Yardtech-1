#include"Camera.h"
#include"console/CVar.h"

glm::vec3 Camera::FlatForward() const {
	glm::vec3 forward{ 0.0f };
	forward.x = cos(glm::radians(yaw));
	forward.y = 0.0f;
	forward.z = sin(glm::radians(yaw));
	return glm::normalize(forward);
}
glm::vec3 Camera::Forward() const {
	glm::vec3 forward{ 0.0f };
	forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	forward.y = sin(glm::radians(pitch));
	forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	return glm::normalize(forward);
}
glm::vec3 Camera::Right() const {
	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	return glm::normalize(glm::cross(Forward(), worldUp));
}

void Camera::MoveForward(float deltaTime) { 
	if (g_CVar.cm_noclip == true) {
		position += Forward() * moveSpeed * deltaTime;
	}
	else {
		position += FlatForward() * moveSpeed * deltaTime;
	}
}
void Camera::MoveBackward(float deltaTime){
	if (g_CVar.cm_noclip == true) {
		position -= Forward() * moveSpeed * deltaTime;
	}
	else {
		position -= FlatForward() * moveSpeed * deltaTime;
	}
}
void Camera::MoveRight(float deltaTime){ 
	position += Right() * moveSpeed * deltaTime;
	if (g_CVar.cm_noclip == false) {
		targetRoll = maxRoll;
	}
}
void Camera::MoveLeft(float deltaTime){ 
	position -= Right() * moveSpeed * deltaTime; 
	if (g_CVar.cm_noclip == false) {
		targetRoll = -maxRoll;
	}
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset) {
	yaw += xOffset * mouseSensitivity;
	pitch -= yOffset * mouseSensitivity;

	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}
}

glm::mat4 Camera::GetViewMatrix() const {
	 return glm::lookAt(position, position + Forward(), Up());
}

glm::vec3 Camera::Up() const {
	glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
	glm::vec3 forward = Forward();
	glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(roll), forward);
	return glm::normalize(glm::vec3(rollMat * glm::vec4(up, 0.0f)));
}

void Camera::Update(float deltaTime) {
	// Yumuşak geçiş: roll -> targetRoll
	roll = glm::mix(roll, targetRoll, glm::clamp(rollLerpSpeed * deltaTime, 0.0f, 1.0f));

	// Bir sonraki frame için sıfırla; eğer o frame'de tekrar
	// MoveRight/MoveLeft çağrılırsa targetRoll tekrar set edilecek.
	targetRoll = 0.0f;
}