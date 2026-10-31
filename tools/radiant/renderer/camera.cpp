//==============================
//	camera.cpp
//==============================

#include "renderer/camera.h"

Camera::Camera()
{
	SetDirection(vec3(1, 0, 0), vec3(0, 0, 1));
}

const mat4 &Camera::ProjectionMatrix() const
{
	if (!valid)
		ValidateMatrices();
	return projectionMatrix;
}

const mat4 &Camera::ViewMatrix() const
{
	if (!valid)
		ValidateMatrices();
	return viewMatrix;
}

mat4 Camera::Matrix() const
{
	if (!valid)
		ValidateMatrices();
	return matrix;
}

void Camera::ZoomBy(float factor)
{
	if (factor > 0)
		SetZoom(zoom * factor);
}

void Camera::SetZoom(float z)
{
	if (z > 0 && z != zoom && IsValidZoom(z))
	{
		zoom = z;
		DoUpdateZoom();
		valid = false;
	}
}

void Camera::SetNearPlane(float n)
{
	if (n < farPlane && n != nearPlane)
	{
		nearPlane = n;
		valid = false;
	}
}

void Camera::SetFarPlane(float f)
{
	if (f > nearPlane && f != farPlane)
	{
		farPlane = f;
		valid = false;
	}
}

bool Camera::SetViewport(const Viewport &v)
{
	if (v == viewport)
		return false;
	viewport = v;
	DoUpdateZoom();
	valid = false;
	return true;
}

void Camera::MoveTo(const vec3 &p)
{
	if (p != position)
	{
		position = p;
		valid = false;
	}
}

void Camera::MoveBy(const vec3 &delta)
{
	if (delta != vec3())
	{
		position += delta;
		valid = false;
	}
}

/*
==================
Camera::SetDirection
==================
*/
void Camera::SetDirection(const vec3 &d, const vec3 &u)
{
	direction = d;

	vec3 r = cross(direction, u);
	if (length(r) < 1e-6f)
	{
		// direction and up were colinear
		vec3 axis;
		axis[abs_max_axis(direction) == 2 ? 0 : 2] = 1;
		r = cross(direction, axis);
	}
	right = normalize(r);
	up = cross(right, direction);
	valid = false;
}

/*
==================
Camera::Project
==================
*/
vec3 Camera::Project(const vec3 &point) const
{
	if (!valid)
		ValidateMatrices();

	vec3 win = matrix * point;
	win.x = (float)(viewport.x + viewport.width) * (win.x + 1.0f) / 2.0f;
	win.y = (float)(viewport.y + viewport.height) * (win.y + 1.0f) / 2.0f;
	win.z = (win.z + 1.0f) / 2.0f;
	return win;
}

/*
==================
Camera::Unproject
==================
*/
vec3 Camera::Unproject(float x, float y, float depth) const
{
	if (!valid)
		ValidateMatrices();

	vec3 normalized;
	normalized.x = 2.0f * (x - (float)viewport.x) / (float)viewport.width - 1.0f;
	normalized.y = 2.0f * ((float)(viewport.height - viewport.y) - y) / (float)viewport.height - 1.0f;
	normalized.z = 2.0f * depth - 1.0f;
	return inverseMatrix * normalized;
}

void Camera::ValidateMatrices() const
{
	DoValidateMatrices(projectionMatrix, viewMatrix);
	matrix = projectionMatrix * viewMatrix;
	invert(matrix, inverseMatrix);
	valid = true;
}
