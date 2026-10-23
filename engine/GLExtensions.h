#pragma once
#include <windows.h>
#include <GL/gl.h>

// Windows'un gl.h'i sadece OpenGL 1.1 deklare ediyor.
// Multitexturing (1.3+ core) ve GLSL (2.0 core) fonksiyon ve sabitlerini elle
// tanimlayip runtime'da wglGetProcAddress ile cekiyoruz.

typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRY* PFNGLMULTITEXCOORD2FPROC)(GLenum target, GLfloat s, GLfloat t);

extern PFNGLACTIVETEXTUREPROC glActiveTexture_;
extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f_;

#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#ifndef GL_COMBINE
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_SOURCE0_RGB 0x8580
#define GL_SOURCE1_RGB 0x8581
#define GL_PREVIOUS 0x8578
#define GL_RGB_SCALE 0x8573
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// --- GLSL (OpenGL 2.0 cekirdegi) ---
// GLchar yerine char kullanilir (ayni bellek duzeni); glext.h eklenmeden derlenir.
typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const char* const* string, const GLint* length);
typedef void (APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);
typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void (APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const char* name);
typedef void (APIENTRY* PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRY* PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRY* PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY* PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY* PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY* PFNGLUNIFORM4FVPROC)(GLint location, GLsizei count, const GLfloat* value);

extern PFNGLCREATESHADERPROC glCreateShader_;
extern PFNGLSHADERSOURCEPROC glShaderSource_;
extern PFNGLCOMPILESHADERPROC glCompileShader_;
extern PFNGLGETSHADERIVPROC glGetShaderiv_;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_;
extern PFNGLDELETESHADERPROC glDeleteShader_;
extern PFNGLCREATEPROGRAMPROC glCreateProgram_;
extern PFNGLATTACHSHADERPROC glAttachShader_;
extern PFNGLLINKPROGRAMPROC glLinkProgram_;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv_;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_;
extern PFNGLUSEPROGRAMPROC glUseProgram_;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram_;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_;
extern PFNGLUNIFORM1IPROC glUniform1i_;
extern PFNGLUNIFORM1FPROC glUniform1f_;
extern PFNGLUNIFORM2FPROC glUniform2f_;
extern PFNGLUNIFORM3FPROC glUniform3f_;
extern PFNGLUNIFORM4FPROC glUniform4f_;
extern PFNGLUNIFORM4FVPROC glUniform4fv_;

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

// Multitexture yuklenemezse false doner (lightmap devre disi kalir).
// GLSL fonksiyonlari ayrica yuklenir; sonucu GLShadersAvailable() verir.
bool LoadGLExtensions();
bool GLShadersAvailable();