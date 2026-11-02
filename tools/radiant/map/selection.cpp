//==============================
//	selection.cpp
//==============================

#include "map/selection.h"
#include "map/map.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

Selection g_selection;

void Selection::Validate()
{
	if (mapRevision != g_map.revision)
	{
		mapRevision = g_map.revision;
		if (!items.empty())
		{
			items.clear();
			revision++;
		}
	}
}

void Selection::Changed()
{
	std::sort(items.begin(), items.end());
	items.erase(std::unique(items.begin(), items.end()), items.end());
	revision++;
}

const std::vector<selItem_t> &Selection::Items()
{
	Validate();
	return items;
}

unsigned Selection::Revision()
{
	Validate();
	return revision;
}

bool Selection::Contains(const selItem_t &item)
{
	Validate();
	return std::binary_search(items.begin(), items.end(), item);
}

void Selection::Clear()
{
	Validate();
	if (items.empty())
		return;
	items.clear();
	revision++;
}

void Selection::Set(const selItem_t &item)
{
	Validate();
	items.assign(1, item);
	Changed();
}

void Selection::Add(const selItem_t &item)
{
	Validate();
	items.push_back(item);
	Changed();
}

void Selection::Remove(const selItem_t &item)
{
	Validate();
	auto it = std::find(items.begin(), items.end(), item);
	if (it == items.end())
		return;
	items.erase(it);
	revision++;
}

void Selection::Toggle(const selItem_t &item)
{
	if (Contains(item))
		Remove(item);
	else
		Add(item);
}

void Selection::AddEntity(int entity)
{
	Validate();
	if (entity < 0 || entity >= (int)g_map.entities.size())
		return;
	const Entity &e = g_map.entities[entity];
	if (e.brushes.empty())
		items.push_back({ entity, -1 });
	for (int b = 0; b < (int)e.brushes.size(); b++)
		if (e.brushes[b].valid)
			items.push_back({ entity, b });
	Changed();
}

// every brush, and every point entity
static std::vector<selItem_t> Everything()
{
	std::vector<selItem_t> all;
	for (int e = 0; e < (int)g_map.entities.size(); e++)
	{
		const Entity &ent = g_map.entities[e];
		dvec3 mins, maxs;
		if (ent.PointBounds(mins, maxs))
			all.push_back({ e, -1 });
		for (int b = 0; b < (int)ent.brushes.size(); b++)
			if (ent.brushes[b].valid)
				all.push_back({ e, b });
	}
	return all;
}

void Selection::SelectAll()
{
	Validate();
	items = Everything();
	Changed();
}

void Selection::Invert()
{
	Validate();
	std::vector<selItem_t> out;
	for (const selItem_t &i : Everything())
		if (!std::binary_search(items.begin(), items.end(), i))
			out.push_back(i);
	items = out;
	Changed();
}

bool Selection::ItemBounds(const selItem_t &item, dvec3 &mins, dvec3 &maxs)
{
	if (item.entity < 0 || item.entity >= (int)g_map.entities.size())
		return false;
	const Entity &e = g_map.entities[item.entity];
	if (item.brush < 0)
		return e.PointBounds(mins, maxs);
	if (item.brush >= (int)e.brushes.size() || !e.brushes[item.brush].valid)
		return false;
	mins = e.brushes[item.brush].mins;
	maxs = e.brushes[item.brush].maxs;
	return true;
}

bool Selection::Bounds(dvec3 &mins, dvec3 &maxs)
{
	bool any = false;
	for (const selItem_t &i : Items())
	{
		dvec3 lo, hi;
		if (!ItemBounds(i, lo, hi))
			continue;
		if (!any)
		{
			mins = lo;
			maxs = hi;
			any = true;
			continue;
		}
		for (int k = 0; k < 3; k++)
		{
			mins[k] = std::min(mins[k], lo[k]);
			maxs[k] = std::max(maxs[k], hi[k]);
		}
	}
	return any;
}

std::string Selection::Describe()
{
	int brushes = 0, entities = 0;
	int lastEntity = -1;
	std::string lastClass;
	for (const selItem_t &i : Items())
	{
		if (i.brush >= 0)
			brushes++;
		if (i.entity != lastEntity && (i.brush < 0 || !g_map.entities[i.entity].IsWorldspawn()))
		{
			entities++;
			lastClass = g_map.entities[i.entity].Classname();
		}
		lastEntity = i.entity;
	}
	if (!brushes && !entities)
		return std::string();

	char text[160];
	std::string out;
	if (brushes)
	{
		snprintf(text, sizeof(text), "%d brush%s", brushes, brushes == 1 ? "" : "es");
		out = text;
	}
	if (entities)
	{
		snprintf(text, sizeof(text), "%s%d entit%s", out.empty() ? "" : ", ", entities, entities == 1 ? "y" : "ies");
		out += text;
		if (entities == 1)
			out += " (" + lastClass + ")";
	}
	return out + " selected";
}

/*
==================
RayBrush

Brush::RayCast with every plane pushed out by grow. a ray starting inside
counts too, at distance 0
==================
*/
static bool RayBrush(const Brush &b, const dvec3 &start, const dvec3 &dir, double grow, double &dist)
{
	double enter = -DBL_MAX, leave = DBL_MAX;
	for (const BrushFace &f : b.faces)
	{
		if (!f.planeValid || f.winding.empty())
			continue;
		double d = f.plane.Distance(start) - grow;
		double denom = dot(f.plane.normal, dir);
		if (fabs(denom) < 1e-9)
		{
			if (d > 0)
				return false;
			continue;
		}
		double t = -d / denom;
		if (denom < 0)
			enter = std::max(enter, t);
		else
			leave = std::min(leave, t);
		if (enter > leave)
			return false;
	}
	if (leave < 0)
		return false;
	dist = std::max(enter, 0.0);
	return true;
}

static bool RayBox(const dvec3 &start, const dvec3 &dir, const dvec3 &mins, const dvec3 &maxs, double grow, double &dist)
{
	double enter = -DBL_MAX, leave = DBL_MAX;
	for (int i = 0; i < 3; i++)
	{
		double lo = mins[i] - grow, hi = maxs[i] + grow;
		if (fabs(dir[i]) < 1e-12)
		{
			if (start[i] < lo || start[i] > hi)
				return false;
			continue;
		}
		double t0 = (lo - start[i]) / dir[i];
		double t1 = (hi - start[i]) / dir[i];
		if (t0 > t1)
			std::swap(t0, t1);
		enter = std::max(enter, t0);
		leave = std::min(leave, t1);
		if (enter > leave)
			return false;
	}
	if (leave < 0)
		return false;
	dist = std::max(enter, 0.0);
	return true;
}

std::vector<pickHit_t> Selection::Pick(const dvec3 &start, const dvec3 &dir, double tolerance)
{
	std::vector<pickHit_t> hits;
	double d;
	dvec3 mins, maxs;

	for (int e = 0; e < (int)g_map.entities.size(); e++)
	{
		const Entity &ent = g_map.entities[e];
		for (int b = 0; b < (int)ent.brushes.size(); b++)
		{
			const Brush &brush = ent.brushes[b];
			if (brush.valid && RayBrush(brush, start, dir, tolerance, d))
				hits.push_back({ { e, b }, d });
		}
		if (ent.PointBounds(mins, maxs) && RayBox(start, dir, mins, maxs, tolerance, d))
			hits.push_back({ { e, -1 }, d });
	}

	std::stable_sort(hits.begin(), hits.end(), [](const pickHit_t &a, const pickHit_t &b) { return a.dist < b.dist; });
	return hits;
}

void Selection::Click(const std::vector<pickHit_t> &hits, bool toggle, bool cycle, bool doubleClick)
{
	if (hits.empty())
	{
		if (!toggle)
			Clear();
		return;
	}

	if (doubleClick)
	{
		// trenchbroom: the whole entity, worldspawn's brushes stay one at a time.
		// the one the first click picked, which may be one cycled to
		selItem_t first = hits.front().item;
		if (Items().size() == 1)
			for (const pickHit_t &h : hits)
				if (h.item == items.front())
					first = h.item;
		if (g_map.entities[first.entity].IsWorldspawn())
		{
			if (!toggle)
				Set(first);
			return;
		}
		if (!toggle)
			Clear();
		AddEntity(first.entity);
		return;
	}

	if (toggle)
	{
		Toggle(hits.front().item);
		return;
	}

	// hammer's and netradiant's cycling: the one after the selected one
	if (cycle && Items().size() == 1)
	{
		for (size_t i = 0; i < hits.size(); i++)
		{
			if (hits[i].item == items.front())
			{
				Set(hits[(i + 1) % hits.size()].item);
				return;
			}
		}
	}
	Set(hits.front().item);
}

void Selection::SelectInside(int axis1, double lo1, double hi1, int axis2, double lo2, double hi2, bool add)
{
	Validate();
	if (!add)
		items.clear();
	for (const selItem_t &i : Everything())
	{
		dvec3 mins, maxs;
		if (!ItemBounds(i, mins, maxs))
			continue;
		if (mins[axis1] >= lo1 && maxs[axis1] <= hi1 && mins[axis2] >= lo2 && maxs[axis2] <= hi2)
			items.push_back(i);
	}
	Changed();
}
