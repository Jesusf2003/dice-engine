//==============================
//	perspectiveCamera.h
//
//	port of trenchbroom's gl::PerspectiveCamera, the 3D view's camera: a field
//	of view instead of a zoom, plus Camera::rotate / orbit and the pick ray.
//	z is always up; the pitch stops short of straight up or down so the view
//	never flips over (trenchbroom clamps the same way)
//==============================

#ifndef __PERSPECTIVE_CAMERA_H__
#define __PERSPECTIVE_CAMERA_H__

#include "renderer/camera.h"

class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera();

	inline float Fov() const	{ return fov; }
	void	SetFov(float fov);				// degrees, horizontal at 4:3 (trenchbroom's convention)

	float	Yaw() const;					// degrees, 0 looks down +x, 90 down +y
	float	Pitch() const;					// degrees, positive looks up
	void	SetAngles(float yaw, float pitch);

	void	Rotate(float yaw, float pitch);	// degrees, around the camera's own position
	void	Orbit(const vec3 &center, float yaw, float pitch);	// position and direction turn around center

	// world space ray through window point x, y (y down from the top)
	void	PickRay(float x, float y, vec3 &origin, vec3 &dir) const;
	// world units one pixel covers at distance dist along the view direction
	float	PixelSize(float dist) const;

private:
	void	DoValidateMatrices(mat4 &projection, mat4 &view) const override;
	void	DoUpdateZoom() override {}
	float	FrustumTop() const;				// half the frustum's height at distance 1

	float	fov = 90.0f;
};

#endif
