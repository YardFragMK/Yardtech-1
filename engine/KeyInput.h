#pragma once
#include<SDL.h>
#include"Camera.h"
#include"map/DynamicLightManager.h"

class KeyInput {
public:
	static void Update(bool& running, Camera& camera, DynamicLightManager& dynamicLights, float deltaTime);
private:

};
