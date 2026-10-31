//==============================
//	orthographicCamera.cpp
//==============================

#include "renderer/orthographicCamera.h"

#include <cmath>

OrthographicCamera::OrthographicCamera()
{
	DoUpdateZoom();
}

void OrthographicCamera::ViewportVertices(vec3 out[4]) const
{
	float w2 = (float)zoomedViewport.width / 2.0f;
	float h2 = (float)zoomedViewport.height / 2.0f;

	out[0] = Position() - w2 * Right() + h2 * Up();
	out[1] = Position() + w2 * Right() + h2 * Up();
	out[2] = Position() + w2 * Right() - h2 * Up();
	out[3] = Position() - w2 * Right() - h2 * Up();
}

void OrthographicCamera::DoValidateMatrices(mat4 &projection, mat4 &view) const
{
	float w2 = (float)zoomedViewport.width / 2.0f;
	float h2 = (float)zoomedViewport.height / 2.0f;

	projection = ortho_matrix(NearPlane(), FarPlane(), -w2, h2, w2, -h2);
	view = view_matrix(Direction(), Up()) * translation_matrix(-Position());
}

void OrthographicCamera::DoUpdateZoom()
{
	const Viewport &v = GetViewport();
	zoomedViewport = Viewport{
		v.x,
		v.y,
		(int)roundf((float)v.width / Zoom()),
		(int)roundf((float)v.height / Zoom()),
	};
}
