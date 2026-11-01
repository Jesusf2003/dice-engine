//==============================
//	perspectiveCamera.cpp
//==============================

#include "renderer/perspectiveCamera.h"

#include <algorithm>
#include <cmath>

#define MAX_PITCH	89.0f

PerspectiveCamera::PerspectiveCamera()
{
	SetNearPlane(1.0f);
}

void PerspectiveCamera::SetFov(float f)
{
	fov = std::clamp(f, 10.0f, 170.0f);
	Invalidate();
}

float PerspectiveCamera::Yaw() const
{
	const vec3 &d = Direction();
	return RAD2DEG(atan2f(d.y, d.x));
}

float PerspectiveCamera::Pitch() const
{
	return RAD2DEG(asinf(std::clamp(Direction().z, -1.0f, 1.0f)));
}

void PerspectiveCamera::SetAngles(float yaw, float pitch)
{
	yaw = DEG2RAD(yaw);
	pitch = DEG2RAD(std::clamp(pitch, -MAX_PITCH, MAX_PITCH));
	SetDirection(vec3(cosf(yaw) * cosf(pitch), sinf(yaw) * cosf(pitch), sinf(pitch)), vec3(0, 0, 1));
}

void PerspectiveCamera::Rotate(float yaw, float pitch)
{
	Orbit(Position(), yaw, pitch);
}

/*
==================
PerspectiveCamera::Orbit

trenchbroom's Camera::orbit: yaw turns around world z, pitch around the
camera's right axis, and the position swings around center by the same turn.
the pitch is clamped first so the direction stays off the vertical
==================
*/
void PerspectiveCamera::Orbit(const vec3 &center, float yaw, float pitch)
{
	float oldPitch = Pitch();
	pitch = std::clamp(oldPitch + pitch, -MAX_PITCH, MAX_PITCH) - oldPitch;

	const vec3 zAxis(0, 0, 1);
	float yawRad = DEG2RAD(yaw);
	float pitchRad = DEG2RAD(pitch);

	vec3 dir = rotate_axis(Direction(), zAxis, yawRad);
	vec3 pitchAxis = normalize(rotate_axis(Right(), zAxis, yawRad));
	vec3 offset = rotate_axis(Position() - center, zAxis, yawRad);

	// a positive turn around right (x forward, -y right) tips the direction up
	dir = rotate_axis(dir, pitchAxis, pitchRad);
	offset = rotate_axis(offset, pitchAxis, pitchRad);

	MoveTo(center + offset);
	SetDirection(normalize(dir), zAxis);
}

void PerspectiveCamera::PickRay(float x, float y, vec3 &origin, vec3 &dir) const
{
	origin = Position();
	dir = normalize(Unproject(x, y, 0.5f) - origin);
}

float PerspectiveCamera::FrustumTop() const
{
	return tanf(DEG2RAD(fov) / 2.0f) * 0.75f;
}

float PerspectiveCamera::PixelSize(float dist) const
{
	const Viewport &v = GetViewport();
	return 2.0f * dist * FrustumTop() / (float)std::max(v.height, 1);
}

/*
==================
PerspectiveCamera::DoValidateMatrices

trenchbroom's perspectiveMatrix: the fov spans 4:3's width, the frustum widens with the viewport
==================
*/
void PerspectiveCamera::DoValidateMatrices(mat4 &projection, mat4 &view) const
{
	const Viewport &v = GetViewport();
	float aspect = (float)std::max(v.width, 1) / (float)std::max(v.height, 1);
	float top = FrustumTop() * NearPlane();
	float side = top * aspect;

	projection = frustum_matrix(-side, side, -top, top, NearPlane(), FarPlane());
	view = view_matrix(Direction(), Up()) * translation_matrix(-Position());
}
