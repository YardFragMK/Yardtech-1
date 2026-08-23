#include "GLExtensions.h"
#include "console/Console.h"

PFNGLACTIVETEXTUREPROC glActiveTexture_ = nullptr;
PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f_ = nullptr;

bool LoadGLExtensions() {
    glActiveTexture_ = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    glMultiTexCoord2f_ = (PFNGLMULTITEXCOORD2FPROC)wglGetProcAddress("glMultiTexCoord2f");

    if (!glActiveTexture_ || !glMultiTexCoord2f_) {
        Console::Log("WARNING-> multitexture yuklenemedi, lightmap devre disi (fullbright)");
        return false;
    }
    Console::Log("Multitexture extension yuklendi.");
    return true;
}