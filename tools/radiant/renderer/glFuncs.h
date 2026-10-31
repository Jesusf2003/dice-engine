//==============================
//	glFuncs.h
//
//	GL 3.3 core entry points. opengl32 only exports GL 1.1, the rest comes from
//	SDL_GL_GetProcAddress once the context exists (GL_LoadFunctions)
//==============================

#ifndef __GL_FUNCS_H__
#define __GL_FUNCS_H__

#include <SDL_opengl.h>

#define GL_FUNC_LIST(X) \
	X(PFNGLCREATESHADERPROC,				glCreateShader) \
	X(PFNGLSHADERSOURCEPROC,				glShaderSource) \
	X(PFNGLCOMPILESHADERPROC,				glCompileShader) \
	X(PFNGLGETSHADERIVPROC,					glGetShaderiv) \
	X(PFNGLGETSHADERINFOLOGPROC,			glGetShaderInfoLog) \
	X(PFNGLDELETESHADERPROC,				glDeleteShader) \
	X(PFNGLCREATEPROGRAMPROC,				glCreateProgram) \
	X(PFNGLATTACHSHADERPROC,				glAttachShader) \
	X(PFNGLBINDATTRIBLOCATIONPROC,			glBindAttribLocation) \
	X(PFNGLLINKPROGRAMPROC,					glLinkProgram) \
	X(PFNGLGETPROGRAMIVPROC,				glGetProgramiv) \
	X(PFNGLGETPROGRAMINFOLOGPROC,			glGetProgramInfoLog) \
	X(PFNGLDELETEPROGRAMPROC,				glDeleteProgram) \
	X(PFNGLUSEPROGRAMPROC,					glUseProgram) \
	X(PFNGLGETUNIFORMLOCATIONPROC,			glGetUniformLocation) \
	X(PFNGLUNIFORM1IPROC,					glUniform1i) \
	X(PFNGLUNIFORM1FPROC,					glUniform1f) \
	X(PFNGLUNIFORM3FPROC,					glUniform3f) \
	X(PFNGLUNIFORM4FPROC,					glUniform4f) \
	X(PFNGLUNIFORMMATRIX4FVPROC,			glUniformMatrix4fv) \
	X(PFNGLGENVERTEXARRAYSPROC,				glGenVertexArrays) \
	X(PFNGLBINDVERTEXARRAYPROC,				glBindVertexArray) \
	X(PFNGLDELETEVERTEXARRAYSPROC,			glDeleteVertexArrays) \
	X(PFNGLGENBUFFERSPROC,					glGenBuffers) \
	X(PFNGLBINDBUFFERPROC,					glBindBuffer) \
	X(PFNGLBUFFERDATAPROC,					glBufferData) \
	X(PFNGLDELETEBUFFERSPROC,				glDeleteBuffers) \
	X(PFNGLENABLEVERTEXATTRIBARRAYPROC,		glEnableVertexAttribArray) \
	X(PFNGLVERTEXATTRIBPOINTERPROC,			glVertexAttribPointer) \
	X(PFNGLGENFRAMEBUFFERSPROC,				glGenFramebuffers) \
	X(PFNGLBINDFRAMEBUFFERPROC,				glBindFramebuffer) \
	X(PFNGLFRAMEBUFFERTEXTURE2DPROC,		glFramebufferTexture2D) \
	X(PFNGLCHECKFRAMEBUFFERSTATUSPROC,		glCheckFramebufferStatus) \
	X(PFNGLDELETEFRAMEBUFFERSPROC,			glDeleteFramebuffers) \
	X(PFNGLGENRENDERBUFFERSPROC,			glGenRenderbuffers) \
	X(PFNGLBINDRENDERBUFFERPROC,			glBindRenderbuffer) \
	X(PFNGLRENDERBUFFERSTORAGEPROC,			glRenderbufferStorage) \
	X(PFNGLFRAMEBUFFERRENDERBUFFERPROC,		glFramebufferRenderbuffer) \
	X(PFNGLDELETERENDERBUFFERSPROC,			glDeleteRenderbuffers) \
	X(PFNGLGENERATEMIPMAPPROC,				glGenerateMipmap)

#define GL_FUNC_DECLARE(type, name)	extern type q##name;
GL_FUNC_LIST(GL_FUNC_DECLARE)
#undef GL_FUNC_DECLARE

bool GL_LoadFunctions();	// false (and logged) if any entry point is missing

#endif
