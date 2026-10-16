#pragma once
#include <vector>
#include "netentity.h"

// Server'in bildirdigi entity durumlarini cizer. Faz 2b'de client.dll'e tasinacak.
void EntityRenderer_Render(const std::vector<NetEntityState>& entities);