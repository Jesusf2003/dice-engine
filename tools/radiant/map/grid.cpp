//==============================
//	grid.cpp
//==============================

#include "map/grid.h"

#include <cmath>

Grid g_grid(3);		// 8 units, quaked3's default

Grid::Grid(int s) : size(s)
{
}

double Grid::ActualSize(int s)
{
	return std::exp2(s);
}

void Grid::SetSize(int s)
{
	if (s >= MinSize && s <= MaxSize)
		size = s;
}

void Grid::IncSize()
{
	if (size < MaxSize)
		++size;
}

void Grid::DecSize()
{
	if (size > MinSize)
		--size;
}

double Grid::ActualSize() const
{
	return snap ? ActualSize(size) : 1.0;
}

void Grid::ToggleVisible()
{
	visible = !visible;
}

void Grid::ToggleSnap()
{
	snap = !snap;
}

double Grid::Snap(double f) const
{
	if (!snap)
		return f;
	double s = ActualSize();
	return s * std::round(f / s);
}

double Grid::SnapUp(double f, bool skip) const
{
	if (!snap)
		return f;
	double a = ActualSize();
	double s = a * std::ceil(f / a);
	return (skip && std::fabs(s - f) < 1e-9) ? s + a : s;
}

double Grid::SnapDown(double f, bool skip) const
{
	if (!snap)
		return f;
	double a = ActualSize();
	double s = a * std::floor(f / a);
	return (skip && std::fabs(s - f) < 1e-9) ? s - a : s;
}
