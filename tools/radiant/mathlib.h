//==============================
//	mathlib.h
//
//	the little vector / matrix math the editor needs, in the spirit of
//	trenchbroom's vm (vec3f, mat4x4f). matrices are column major, like GL
//==============================

#ifndef __MATHLIB_H__
#define __MATHLIB_H__

#include <cmath>

/*
==================
vec3
==================
*/
struct vec3
{
	float x = 0, y = 0, z = 0;

	vec3() {}
	vec3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}

	float &operator[](int i)				{ return (&x)[i]; }
	const float &operator[](int i) const	{ return (&x)[i]; }

	vec3 operator+(const vec3 &o) const		{ return vec3(x + o.x, y + o.y, z + o.z); }
	vec3 operator-(const vec3 &o) const		{ return vec3(x - o.x, y - o.y, z - o.z); }
	vec3 operator-() const					{ return vec3(-x, -y, -z); }
	vec3 operator*(float s) const			{ return vec3(x * s, y * s, z * s); }
	vec3 operator/(float s) const			{ return vec3(x / s, y / s, z / s); }
	vec3 &operator+=(const vec3 &o)			{ x += o.x; y += o.y; z += o.z; return *this; }
	vec3 &operator-=(const vec3 &o)			{ x -= o.x; y -= o.y; z -= o.z; return *this; }
	bool operator==(const vec3 &o) const	{ return x == o.x && y == o.y && z == o.z; }
	bool operator!=(const vec3 &o) const	{ return !(*this == o); }
};

inline vec3 operator*(float s, const vec3 &v)	{ return v * s; }

inline float dot(const vec3 &a, const vec3 &b)	{ return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3 &a, const vec3 &b)
{
	return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline float length(const vec3 &v)				{ return sqrtf(dot(v, v)); }
inline vec3 normalize(const vec3 &v)			{ float l = length(v); return l > 0 ? v / l : v; }

// index of the component with the largest magnitude: 0 = x, 1 = y, 2 = z
inline int abs_max_axis(const vec3 &v)
{
	float ax = fabsf(v.x), ay = fabsf(v.y), az = fabsf(v.z);
	if (ax >= ay && ax >= az) return 0;
	return ay >= az ? 1 : 2;
}

/*
==================
mat4

column major: m[col * 4 + row]
==================
*/
struct mat4
{
	float m[16];

	mat4() { for (int i = 0; i < 16; i++) m[i] = (i % 5 == 0) ? 1.0f : 0.0f; }

	float &at(int row, int col)				{ return m[col * 4 + row]; }
	float at(int row, int col) const		{ return m[col * 4 + row]; }

	mat4 operator*(const mat4 &o) const
	{
		mat4 r;
		for (int c = 0; c < 4; c++)
			for (int rr = 0; rr < 4; rr++)
			{
				float s = 0;
				for (int k = 0; k < 4; k++)
					s += at(rr, k) * o.at(k, c);
				r.at(rr, c) = s;
			}
		return r;
	}

	// point transform with the perspective divide
	vec3 operator*(const vec3 &v) const
	{
		float x = at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z + at(0, 3);
		float y = at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z + at(1, 3);
		float z = at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z + at(2, 3);
		float w = at(3, 0) * v.x + at(3, 1) * v.y + at(3, 2) * v.z + at(3, 3);
		return (w != 0 && w != 1) ? vec3(x / w, y / w, z / w) : vec3(x, y, z);
	}
};

inline mat4 translation_matrix(const vec3 &t)
{
	mat4 r;
	r.at(0, 3) = t.x;
	r.at(1, 3) = t.y;
	r.at(2, 3) = t.z;
	return r;
}

// glOrtho
inline mat4 ortho_matrix(float nearPlane, float farPlane, float left, float top, float right, float bottom)
{
	mat4 r;
	r.at(0, 0) = 2.0f / (right - left);
	r.at(1, 1) = 2.0f / (top - bottom);
	r.at(2, 2) = -2.0f / (farPlane - nearPlane);
	r.at(0, 3) = -(right + left) / (right - left);
	r.at(1, 3) = -(top + bottom) / (top - bottom);
	r.at(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
	return r;
}

// camera looking along direction with the given up: rows are right, up, -direction
inline mat4 view_matrix(const vec3 &direction, const vec3 &up)
{
	vec3 r = normalize(cross(direction, up));
	vec3 u = cross(r, direction);
	mat4 m;
	m.at(0, 0) = r.x;			m.at(0, 1) = r.y;			m.at(0, 2) = r.z;
	m.at(1, 0) = u.x;			m.at(1, 1) = u.y;			m.at(1, 2) = u.z;
	m.at(2, 0) = -direction.x;	m.at(2, 1) = -direction.y;	m.at(2, 2) = -direction.z;
	return m;
}

// general 4x4 inverse (cofactors), false if singular
inline bool invert(const mat4 &in, mat4 &out)
{
	const float *a = in.m;
	float inv[16];

	inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
	inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
	inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
	inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
	inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
	inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
	inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
	inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
	inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
	inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
	inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
	inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
	inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
	inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
	inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
	inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

	float det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
	if (det == 0)
		return false;

	det = 1.0f / det;
	for (int i = 0; i < 16; i++)
		out.m[i] = inv[i] * det;
	return true;
}

#endif
