//==============================
//	grid.h
//
//	port of trenchbroom's mdl::Grid: the grid size as a power of two (-3 = 1/8
//	unit, 8 = 256 units), snapping on/off and visibility, plus the scalar snap
//	helpers. vector snapping, ray intersection and the move deltas come with the
//	map tools
//==============================

#ifndef __GRID_H__
#define __GRID_H__

#define MAX_WORLD_COORD	32768.0f	// map bounds: -MAX..MAX on every axis

class Grid
{
public:
	static const int MaxSize = 8;
	static const int MinSize = -3;

	explicit Grid(int size);

	static double ActualSize(int size);		// 2^size

	inline int Size() const		{ return size; }
	void	SetSize(int size);
	void	IncSize();
	void	DecSize();
	double	ActualSize() const;				// snap step: 1 when snapping is off

	inline bool Visible() const	{ return visible; }
	void	ToggleVisible();
	inline bool Snap() const	{ return snap; }
	void	ToggleSnap();

	double	Snap(double f) const;					// nearest grid increment
	double	SnapUp(double f, bool skip) const;		// skip: already on the grid goes one step further
	double	SnapDown(double f, bool skip) const;

private:
	int		size;
	bool	snap = true;
	bool	visible = true;
};

// TODO: trenchbroom keeps the grid in the map document, it moves there once the map is ported
extern Grid g_grid;

#endif
