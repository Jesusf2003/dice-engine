#pragma once
//3D vector in map space. Doubles, since .map plane points and windings need the precision.

#include <algorithm>
#include <cmath>

namespace radiant {

struct Vec3
{
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;

	constexpr Vec3() = default;
	constexpr Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

	constexpr double operator[](int axis) const { return axis == 0 ? x : axis == 1 ? y : z; }
	constexpr double &operator[](int axis) { return axis == 0 ? x : axis == 1 ? y : z; }

	constexpr Vec3 &operator+=(const Vec3 &o) { x += o.x; y += o.y; z += o.z; return *this; }
	constexpr Vec3 &operator-=(const Vec3 &o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	constexpr Vec3 &operator*=(double s) { x *= s; y *= s; z *= s; return *this; }
	constexpr Vec3 &operator/=(double s) { x /= s; y /= s; z /= s; return *this; }
};

constexpr Vec3 operator+(Vec3 a, const Vec3 &b) { return a += b; }
constexpr Vec3 operator-(Vec3 a, const Vec3 &b) { return a -= b; }
constexpr Vec3 operator-(const Vec3 &a) { return {-a.x, -a.y, -a.z}; }
constexpr Vec3 operator*(Vec3 a, double s) { return a *= s; }
constexpr Vec3 operator*(double s, Vec3 a) { return a *= s; }
constexpr Vec3 operator/(Vec3 a, double s) { return a /= s; }

//Exact comparison; use nearlyEqual for anything that went through arithmetic.
constexpr bool operator==(const Vec3 &a, const Vec3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
constexpr bool operator!=(const Vec3 &a, const Vec3 &b) { return !(a == b); }

constexpr double dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
constexpr Vec3 cross(const Vec3 &a, const Vec3 &b)
{
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
constexpr double lengthSquared(const Vec3 &v) { return dot(v, v); }
inline double length(const Vec3 &v) { return std::sqrt(lengthSquared(v)); }

//Returns the zero vector for zero-length input instead of dividing by zero.
inline Vec3 normalized(const Vec3 &v)
{
	const double len = length(v);
	return len > 0.0 ? v / len : Vec3{};
}

inline bool nearlyEqual(const Vec3 &a, const Vec3 &b, double epsilon)
{
	return std::fabs(a.x - b.x) <= epsilon && std::fabs(a.y - b.y) <= epsilon && std::fabs(a.z - b.z) <= epsilon;
}

inline Vec3 componentMin(const Vec3 &a, const Vec3 &b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
inline Vec3 componentMax(const Vec3 &a, const Vec3 &b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }

} // namespace radiant
