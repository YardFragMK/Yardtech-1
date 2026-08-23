#pragma once
#include <windows.h>
#include <GL/gl.h>

// Windows'un gl.h'i sadece OpenGL 1.1 deklare ediyor.
// Multitexturing (1.3+ core) fonksiyon ve sabitlerini elle tanimlayip
// runtime'da wglGetProcAddress ile cekiyoruz.

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

// glContext current olduktan sonra bir kere cagir (Renderer::Init icinde)
bool LoadGLExtensions();