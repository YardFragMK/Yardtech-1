#include "Time.h"

float Time::s_DeltaTime = 0.0f;
float Time::s_TotalTime = 0.0f;

void Time::Update(float deltaTime) {
	s_DeltaTime = deltaTime;
	s_TotalTime += deltaTime;
}
float Time::DeltaTime() {
	return s_DeltaTime;
}
float Time::TotalTime() {
	return s_TotalTime;
}