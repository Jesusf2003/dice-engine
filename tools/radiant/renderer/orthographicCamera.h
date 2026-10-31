//==============================
//	orthographicCamera.h
//
//	port of trenchbroom's gl::OrthographicCamera, the 2D views' camera. zoom
//	shrinks the viewport in world units: at zoom 1 one pixel is one unit
//==============================

#ifndef __ORTHOGRAPHIC_CAMERA_H__
#define __ORTHOGRAPHIC_CAMERA_H__

#include "renderer/camera.h"

class OrthographicCamera : public Camera
{
public:
	OrthographicCamera();

	inline const Viewport &ZoomedViewport() const	{ return zoomedViewport; }

	// the viewport rectangle in world space, top left first, clockwise
	void	ViewportVertices(vec3 out[4]) const;

private:
	void	DoValidateMatrices(mat4 &projection, mat4 &view) const override;
	void	DoUpdateZoom() override;

	Viewport	zoomedViewport;
};

#endif
