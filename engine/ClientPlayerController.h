#pragma once
#include <glm/glm.hpp>

// Client'a ozgu katman: input'u SDL'den okur, PlayerMovement.h'daki saf
// SimulatePlayerPhysics'i cagirir, sonucu gorsel global'lere (g_Camera) yazar.
// Server bu dosyayi hic gormez -- kendi input kaynagini (agdan gelen komutlar)
// kullanarak ayni SimulatePlayerPhysics'i kendi tarafinda cagirir.
void UpdatePlayerPhysics(float deltaTime, glm::vec3 oldPos);