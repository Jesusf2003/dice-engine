//==============================
//	selection.h
//
//	what's selected in g_map, and picking it from the views. the behaviour is
//	put together from the three editors this one follows:
//
//	-	a brush is selected on its own, also inside a brush entity; a point
//		entity is selected as a whole (trenchbroom). double clicking a brush of
//		a brush entity selects all of the entity's brushes (trenchbroom's group)
//	-	a click selects what's under the cursor, control + click adds it or
//		takes it away (hammer, trenchbroom)
//	-	in the 2D views everything along the depth axis is under the cursor:
//		clicking again on the same spot steps to the next one behind it
//		(hammer, netradiant's selection cycling)
//	-	dragging a box in a 2D view selects what's entirely inside it
//		(netradiant's select inside, hammer's marquee)
//
//	the selection is cleared whenever the map is replaced (g_map.revision)
//==============================

#ifndef __SELECTION_H__
#define __SELECTION_H__

#include "mathlib.h"

#include <string>
#include <vector>

struct selItem_t
{
	int		entity;
	int		brush;		// -1: the point entity itself

	bool operator==(const selItem_t &o) const	{ return entity == o.entity && brush == o.brush; }
	bool operator<(const selItem_t &o) const	{ return entity != o.entity ? entity < o.entity : brush < o.brush; }
};

struct pickHit_t
{
	selItem_t	item;
	double		dist;	// along the ray
};

class Selection
{
public:
	const std::vector<selItem_t> &Items();		// sorted
	unsigned	Revision();						// changes with every change, the renderers follow it
	bool	Empty()		{ return Items().empty(); }
	bool	Contains(const selItem_t &item);

	void	Clear();
	void	Set(const selItem_t &item);			// just this one
	void	Add(const selItem_t &item);
	void	Remove(const selItem_t &item);
	void	Toggle(const selItem_t &item);
	void	AddEntity(int entity);				// every brush of it, or the point entity
	void	SelectAll();						// everything but worldspawn itself
	void	Invert();

	// the box around what's selected; false when nothing is
	bool	Bounds(dvec3 &mins, dvec3 &maxs);
	// "2 brushes, 1 entity (light)": for the status bar
	std::string	Describe();

	/*
		everything the ray goes through, nearest first. tolerance grows every
		brush and box by that much: a 2D view's few pixels, so a wall seen
		edge on is still easy to hit
	*/
	static std::vector<pickHit_t>	Pick(const dvec3 &start, const dvec3 &dir, double tolerance);
	// whether it's in the map, and its box
	static bool	ItemBounds(const selItem_t &item, dvec3 &mins, dvec3 &maxs);

	/*
		a click on hits (Pick's, in the order to take them): the first one, or
		with toggle (control) the first one added or taken away. cycle (the
		same spot clicked again) steps from the one selected to the one after
		it. a double click selects the first one's whole entity. nothing hit
		clears the selection, unless toggling
	*/
	void	Click(const std::vector<pickHit_t> &hits, bool toggle, bool cycle, bool doubleClick);
	// a 2D view's box: what's entirely inside it along the two axes (add: kept with the selection)
	void	SelectInside(int axis1, double lo1, double hi1, int axis2, double lo2, double hi2, bool add);

private:
	void	Validate();
	void	Changed();

	std::vector<selItem_t>	items;
	unsigned	revision = 1;
	unsigned	mapRevision = 0;
};

extern Selection g_selection;

#endif
