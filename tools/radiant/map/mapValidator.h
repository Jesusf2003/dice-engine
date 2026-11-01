//==============================
//	mapValidator.h
//
//	checks a parsed map before the editor trusts it, everything found goes to
//	the console through MapReport:
//		entities	worldspawn first and only once, classnames, origins, values
//					the engine can't read (over 1023 characters)
//		wads		worldspawn's wad list: absolute paths, missing files, repeats
//		brushes		built from their planes like qbsp: degenerate, repeated and
//					redundant faces, brushes that enclose nothing, out of bounds
//		textures	names missing from the loaded wads, bad scales and axes
//		targets		target / killtarget naming no targetname
//	what runs is up to Console > Advanced; the detail lines too (verbose: per
//	entity, debug: per brush and face)
//==============================

#ifndef __MAP_VALIDATOR_H__
#define __MAP_VALIDATOR_H__

#include <string>

class Map;
class MapReport;

namespace MapValidator
{
	// mapDir: the .map's folder, relative wad paths are looked up there too. fills map.stats
	void	Validate(Map &map, MapReport &report, const std::string &mapDir);
}

#endif
