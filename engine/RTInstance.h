#pragma once
#include <RTGL1.h>
#include "RTRenderer.h"

// g_RTRenderer.GetRawInstance()'i (void*) her cagiran yerde tek tek cast
// etmek yerine, tek bir yerden tipli erisim saglar.
inline RgInstance GetRTInstance() {
    return static_cast<RgInstance>(g_RTRenderer.GetRawInstance());
}