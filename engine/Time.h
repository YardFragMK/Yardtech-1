#pragma once


class Time {
public:
	static void Update(float deltaTime);
	static float DeltaTime();
	static float TotalTime();


private:
	static float s_DeltaTime;
	static float s_TotalTime;
};