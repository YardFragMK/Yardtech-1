#include "GLExtensions.h"
#include "console/Console.h"
#include <cstdint>

PFNGLACTIVETEXTUREPROC glActiveTexture_ = nullptr;
PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f_ = nullptr;

PFNGLCREATESHADERPROC glCreateShader_ = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource_ = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader_ = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv_ = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ = nullptr;
PFNGLDELETESHADERPROC glDeleteShader_ = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram_ = nullptr;
PFNGLATTACHSHADERPROC glAttachShader_ = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram_ = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv_ = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram_ = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram_ = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ = nullptr;
PFNGLUNIFORM1IPROC glUniform1i_ = nullptr;
PFNGLUNIFORM1FPROC glUniform1f_ = nullptr;
PFNGLUNIFORM2FPROC glUniform2f_ = nullptr;
PFNGLUNIFORM3FPROC glUniform3f_ = nullptr;
PFNGLUNIFORM4FPROC glUniform4f_ = nullptr;
PFNGLUNIFORM4FVPROC glUniform4fv_ = nullptr;

namespace {
    bool s_shadersAvailable = false;

    // Bazi surucular basarisizlikta 0 yerine 1, 2, 3 veya -1 dondurur.
    template <typename T>
    bool LoadProc(T& target, const char* name) {
        const uintptr_t address = reinterpret_cast<uintptr_t>(wglGetProcAddress(name));
        if (address <= 3 || address == static_cast<uintptr_t>(-1)) {
            target = nullptr;
            return false;
        }
        target = reinterpret_cast<T>(address);
        return true;
    }

    bool LoadShaderFunctions() {
        bool ok = true;
        ok &= LoadProc(glCreateShader_, "glCreateShader");
        ok &= LoadProc(glShaderSource_, "glShaderSource");
        ok &= LoadProc(glCompileShader_, "glCompileShader");
        ok &= LoadProc(glGetShaderiv_, "glGetShaderiv");
        ok &= LoadProc(glGetShaderInfoLog_, "glGetShaderInfoLog");
        ok &= LoadProc(glDeleteShader_, "glDeleteShader");
        ok &= LoadProc(glCreateProgram_, "glCreateProgram");
        ok &= LoadProc(glAttachShader_, "glAttachShader");
        ok &= LoadProc(glLinkProgram_, "glLinkProgram");
        ok &= LoadProc(glGetProgramiv_, "glGetProgramiv");
        ok &= LoadProc(glGetProgramInfoLog_, "glGetProgramInfoLog");
        ok &= LoadProc(glUseProgram_, "glUseProgram");
        ok &= LoadProc(glDeleteProgram_, "glDeleteProgram");
        ok &= LoadProc(glGetUniformLocation_, "glGetUniformLocation");
        ok &= LoadProc(glUniform1i_, "glUniform1i");
        ok &= LoadProc(glUniform1f_, "glUniform1f");
        ok &= LoadProc(glUniform2f_, "glUniform2f");
        ok &= LoadProc(glUniform3f_, "glUniform3f");
        ok &= LoadProc(glUniform4f_, "glUniform4f");
        ok &= LoadProc(glUniform4fv_, "glUniform4fv");
        return ok;
    }
}

bool LoadGLExtensions() {
    glActiveTexture_ = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
    glMultiTexCoord2f_ = (PFNGLMULTITEXCOORD2FPROC)wglGetProcAddress("glMultiTexCoord2f");

    s_shadersAvailable = LoadShaderFunctions();
    Console::Log(s_shadersAvailable ? "GLSL shader destegi yuklendi." : "WARNING-> GLSL yuklenemedi, shader efektleri devre disi.");

    if (!glActiveTexture_ || !glMultiTexCoord2f_) {
        Console::Log("WARNING-> multitexture yuklenemedi, lightmap devre disi (fullbright)");
        return false;
    }
    Console::Log("Multitexture extension yuklendi.");
    return true;
}

bool GLShadersAvailable() {
    return s_shadersAvailable;
}