#pragma once
#include <glm/glm.hpp>

// Yercekimi, ziplama, egilme (crouch) ve dunya ile collision cozumlemesini yapar.
// oldPos: bu frame'in basindaki (input islenmeden onceki) kamera pozisyonu.
void UpdatePlayerPhysics(float deltaTime, glm::vec3 oldPos);