//==============================
//	gridRenderer.cpp
//==============================

#include "renderer/gridRenderer.h"
#include "renderer/glFuncs.h"
#include "renderer/orthographicCamera.h"
#include "map/grid.h"

GridRenderer g_gridRenderer;

/*
========================================================================

	SHADERS

	trenchbroom's Grid2D.vertsh, Grid2D.fragsh, Grid.fragsh and GridCommon.fragsh,
	moved from GLSL 1.20 to 3.30 core
========================================================================
*/

static const char *s_gridVertex = R"(#version 330 core
uniform mat4 ModelViewProjection;
in vec3 position;
out vec3 modelCoordinates;

void main()
{
	gl_Position = ModelViewProjection * vec4(position, 1.0);
	modelCoordinates = position;
}
)";

static const char *s_gridFragment = R"(#version 330 core
uniform bool RenderGrid;
uniform vec4 GridColor;
uniform float GridSize;
uniform float GridAlpha;
uniform float CameraZoom;
uniform vec3 Normal;

in vec3 modelCoordinates;
out vec4 fragColor;

// GridCommon.fragsh: soft coverage of a grid line, major lines thicker and stronger
float gridLine(float coord, float stripeSize, float isMajor)
{
	// antialiasing half-width, in grid-cell units
	float edge = 2.0 * fwidth(coord);
	// triangle wave: 1.0 on a grid line, 0.0 exactly between two adjacent lines
	float triangle = abs(2.0 * fract(coord) - 1.0);

	float majorWidthScale = 1.5;
	float majorStripeSize = 1.0 - majorWidthScale * (1.0 - stripeSize);
	float effectiveStripeSize = mix(stripeSize, majorStripeSize, isMajor);
	float stripe = smoothstep(effectiveStripeSize - edge, effectiveStripeSize + edge, triangle);

	float minorOpacity = 0.65;
	float majorOpacity = 1.0;
	return stripe * mix(minorOpacity, majorOpacity, isMajor);
}

// Grid.fragsh
float getSoftStripes(float value, float gridRatio, float stripeSize)
{
	float coord = value * gridRatio;

	// major line every 64 units
	float mValue = 1.0 / (64.0 * gridRatio);
	float triMajor = abs(2.0 * fract(coord * mValue) - 1.0);
	float isMajor = step(1.0 - mValue, triMajor);

	return gridLine(coord, stripeSize, isMajor);
}

float gridLinesSoft(vec2 inCoords, float gridRatio, float gridRatio2, float lineWidth, float gridBlend)
{
	float stripeSize = 1.0 - lineWidth * gridRatio;
	float stripeSize2 = 1.0 - lineWidth * gridRatio2;

	float theGrid = max(getSoftStripes(inCoords.x, gridRatio, stripeSize), getSoftStripes(inCoords.y, gridRatio, stripeSize));
	float nextGrid = max(getSoftStripes(inCoords.x, gridRatio2, stripeSize2), getSoftStripes(inCoords.y, gridRatio2, stripeSize2));

	return mix(theGrid, nextGrid, gridBlend);
}

float gridNextLarger(float size)
{
	return 2.0 * size;
}

float gridFloor(float size)
{
	return exp2(floor(log2(size)));
}

float grid(vec3 coords, vec3 normal, float gridSize, float minGridSize, float lineWidthFactor)
{
	float lineWidth = (gridSize < 1.0 ? (1.0 / 32.0) : (gridSize < 4.0 ? 0.25 : 0.5)) * lineWidthFactor;

	// magic number to make the grid fade sooner, preventing aliasing
	float minGridSizeToRender = minGridSize * 2.0;

	float baseGridSize = gridFloor(minGridSizeToRender);
	if (gridSize > baseGridSize)
		baseGridSize = gridSize;
	float nextGridSize = gridNextLarger(baseGridSize);

	// 0 renders just baseGridSize, 1 fully renders nextGridSize
	float gridBlend = smoothstep(baseGridSize, nextGridSize, minGridSizeToRender);

	vec2 baseCoords;
	if (abs(normal.x) > abs(normal.y))
		baseCoords = abs(normal.x) > abs(normal.z) ? coords.yz : coords.xy;
	else
		baseCoords = abs(normal.y) > abs(normal.z) ? coords.xz : coords.xy;

	return gridLinesSoft(baseCoords, 1.0 / baseGridSize, 1.0 / nextGridSize, lineWidth, gridBlend);
}

// Grid2D.fragsh
void main()
{
	if (!RenderGrid || GridAlpha <= 0.0)
		discard;

	float minGridSize = 5.0 / CameraZoom;
	float lineWidthFactor = 2.0 / CameraZoom;
	float gridValue = grid(modelCoordinates, Normal, GridSize, minGridSize, lineWidthFactor);
	fragColor = vec4(GridColor.rgb, gridValue * GridAlpha);
}
)";


/*
========================================================================

	RENDERER

========================================================================
*/

GridRenderer::~GridRenderer()
{
	Shutdown();
}

bool GridRenderer::Init()
{
	if (!shader.Build("grid2d", s_gridVertex, s_gridFragment))
		return false;

	qglGenVertexArrays(1, &vao);
	qglGenBuffers(1, &vbo);
	qglBindVertexArray(vao);
	qglBindBuffer(GL_ARRAY_BUFFER, vbo);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * 4, nullptr, GL_DYNAMIC_DRAW);
	qglEnableVertexAttribArray(0);
	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
	qglBindVertexArray(0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	return true;
}

void GridRenderer::Shutdown()
{
	if (vbo) qglDeleteBuffers(1, &vbo);
	if (vao) qglDeleteVertexArrays(1, &vao);
	vao = vbo = 0;
	shader.Destroy();
}

/*
==================
GridRenderer::Render

the quad sits on the far side of the world bounds, behind everything else in the view
==================
*/
void GridRenderer::Render(const OrthographicCamera &camera, float gridSize, const float color[4], float alpha)
{
	if (!shader.Valid())
		return;

	const Camera::Viewport &viewport = camera.ZoomedViewport();
	float w = (float)viewport.width / 2.0f;
	float h = (float)viewport.height / 2.0f;
	const vec3 &p = camera.Position();
	vec3 v[4];

	switch (abs_max_axis(camera.Direction()))
	{
	case 0:
		v[0] = vec3(-MAX_WORLD_COORD, p.y - w, p.z - h);
		v[1] = vec3(-MAX_WORLD_COORD, p.y - w, p.z + h);
		v[2] = vec3(-MAX_WORLD_COORD, p.y + w, p.z + h);
		v[3] = vec3(-MAX_WORLD_COORD, p.y + w, p.z - h);
		break;
	case 1:
		v[0] = vec3(p.x - w, MAX_WORLD_COORD, p.z - h);
		v[1] = vec3(p.x - w, MAX_WORLD_COORD, p.z + h);
		v[2] = vec3(p.x + w, MAX_WORLD_COORD, p.z + h);
		v[3] = vec3(p.x + w, MAX_WORLD_COORD, p.z - h);
		break;
	default:
		v[0] = vec3(p.x - w, p.y - h, -MAX_WORLD_COORD);
		v[1] = vec3(p.x - w, p.y + h, -MAX_WORLD_COORD);
		v[2] = vec3(p.x + w, p.y + h, -MAX_WORLD_COORD);
		v[3] = vec3(p.x + w, p.y - h, -MAX_WORLD_COORD);
		break;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	shader.Use();
	shader.Set("ModelViewProjection", camera.Matrix());
	shader.Set("Normal", -camera.Direction());
	shader.Set("RenderGrid", 1);
	shader.Set("GridSize", gridSize);
	shader.Set("GridAlpha", alpha);
	shader.Set("GridColor", color[0], color[1], color[2], color[3]);
	shader.Set("CameraZoom", camera.Zoom());

	qglBindVertexArray(vao);
	qglBindBuffer(GL_ARRAY_BUFFER, vbo);
	qglBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	qglBindVertexArray(0);
	qglBindBuffer(GL_ARRAY_BUFFER, 0);
	qglUseProgram(0);
}
