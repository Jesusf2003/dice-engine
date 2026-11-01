//==============================
//	map.h
//
//	the map document: its entities (worldspawn first) and where it came from.
//	Load reads a .map, checks it and reports the whole process to the console
//	(map/mapParser, map/mapValidator). the 3D view draws it (renderer/mapRenderer).
//	TODO: save, undo, selection, and the views once loading is trusted
//==============================

#ifndef __MAP_H__
#define __MAP_H__

#include "map/entity.h"

#include <string>
#include <vector>

enum MapFormat
{
	MAPFORMAT_UNKNOWN,		// nothing with faces in it yet
	MAPFORMAT_STANDARD,		// quake's: TEX xoff yoff rot xscale yscale
	MAPFORMAT_VALVE220,		// mapversion 220: texture axes in [ ]
	MAPFORMAT_MIXED,		// some faces of each, qbsp copes but it's suspicious
};

const char *MapFormatName(MapFormat format);

struct mapStats_t
{
	int		pointEntities = 0, brushEntities = 0;
	int		worldBrushes = 0, entityBrushes = 0, invalidBrushes = 0;
	int		faces = 0;
	int		textures = 0, missingTextures = 0;
	dvec3	mins, maxs;			// of the valid brushes
	bool	hasBounds = false;
};

class Map
{
public:
	Map();

	std::string			filename;		// full path, empty for a new map
	MapFormat			format = MAPFORMAT_UNKNOWN;
	std::vector<Entity>	entities;
	mapStats_t			stats;			// from the last load
	unsigned			revision = 0;	// changes with every New / Load, renderers rebuild when it does

	void	New();							// just a worldspawn
	bool	Load(const std::string &path);	// replaces this map only if the file parses
	Entity	*World();
	std::string	Name() const;			// the file name without its folder, "unnamed" when new

	// the nearest brush or point entity box along dir (unit) from start
	bool	RayCast(const dvec3 &start, const dvec3 &dir, double &dist) const;
	const Entity	*FindEntity(const std::string &classname) const;	// the first one
};

extern Map g_map;

#endif
