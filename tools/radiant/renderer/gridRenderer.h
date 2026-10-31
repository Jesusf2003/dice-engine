//==============================
//	gridRenderer.h
//
//	port of trenchbroom's render::GridRenderer and its Grid2D shader: one quad
//	covering the 2D view, the fragment shader draws antialiased grid lines,
//	major lines every 64 units, and fades to the next coarser grid as the view
//	zooms out instead of turning into a solid sheet
//==============================

#ifndef __GRID_RENDERER_H__
#define __GRID_RENDERER_H__

#include "renderer/shader.h"

class OrthographicCamera;

class GridRenderer
{
public:
	GridRenderer() {}
	~GridRenderer();

	bool	Init();			// needs the GL context and GL_LoadFunctions
	void	Shutdown();

	// gridSize in world units; alpha scales the line opacity
	void	Render(const OrthographicCamera &camera, float gridSize, const float color[4], float alpha);

private:
	Shader			shader;
	unsigned int	vao = 0, vbo = 0;
};

extern GridRenderer g_gridRenderer;	// shared by the 2D views, one GL context

#endif
