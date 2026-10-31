//==============================
//	glFuncs.cpp
//==============================

#include "renderer/glFuncs.h"

#include <SDL.h>

#define GL_FUNC_DEFINE(type, name)	type q##name = nullptr;
GL_FUNC_LIST(GL_FUNC_DEFINE)
#undef GL_FUNC_DEFINE

/*
==================
GL_LoadFunctions
==================
*/
bool GL_LoadFunctions()
{
	bool ok = true;

#define GL_FUNC_LOAD(type, name) \
	q##name = (type)SDL_GL_GetProcAddress(#name); \
	if (!q##name) \
	{ \
		SDL_Log("GL: missing %s", #name); \
		ok = false; \
	}
	GL_FUNC_LIST(GL_FUNC_LOAD)
#undef GL_FUNC_LOAD

	return ok;
}
