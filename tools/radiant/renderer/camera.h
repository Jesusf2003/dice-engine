//==============================
//	camera.h
//
//	port of trenchbroom's gl::Camera: position, orientation, viewport and zoom,
//	the projection / view matrices and (un)projection between window and world.
//	window coordinates have y going down from the top of the viewport
//==============================

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "mathlib.h"

class Camera
{
public:
	struct Viewport
	{
		int x = 0, y = 0, width = 0, height = 0;

		bool operator==(const Viewport &o) const	{ return x == o.x && y == o.y && width == o.width && height == o.height; }
		bool operator!=(const Viewport &o) const	{ return !(*this == o); }
	};

	virtual ~Camera() {}

	inline float	NearPlane() const				{ return nearPlane; }
	inline float	FarPlane() const				{ return farPlane; }
	inline const Viewport &GetViewport() const		{ return viewport; }
	inline float	Zoom() const					{ return zoom; }
	inline const vec3 &Position() const				{ return position; }
	inline const vec3 &Direction() const			{ return direction; }
	inline const vec3 &Up() const					{ return up; }
	inline const vec3 &Right() const				{ return right; }

	const mat4		&ProjectionMatrix() const;
	const mat4		&ViewMatrix() const;
	mat4			Matrix() const;					// projection * view

	void	ZoomBy(float factor);
	void	SetZoom(float zoom);
	void	SetNearPlane(float nearPlane);
	void	SetFarPlane(float farPlane);
	bool	SetViewport(const Viewport &viewport);
	void	MoveTo(const vec3 &position);
	void	MoveBy(const vec3 &delta);
	void	SetDirection(const vec3 &direction, const vec3 &up);

	vec3	Project(const vec3 &point) const;					// world -> window (y up), depth 0..1
	vec3	Unproject(float x, float y, float depth) const;		// window (y down) -> world

protected:
	Camera();

	virtual void	DoValidateMatrices(mat4 &projection, mat4 &view) const = 0;
	virtual void	DoUpdateZoom() = 0;
	virtual bool	IsValidZoom(float z) const	{ return z >= 0.02f && z <= 100.0f; }

private:
	void	ValidateMatrices() const;

	float		nearPlane = 1.0f;
	float		farPlane = 65536.0f;
	Viewport	viewport = { 0, 0, 1024, 768 };
	float		zoom = 1.0f;
	vec3		position, direction, up, right;

	mutable mat4	projectionMatrix, viewMatrix, matrix, inverseMatrix;
	mutable bool	valid = false;
};

#endif
