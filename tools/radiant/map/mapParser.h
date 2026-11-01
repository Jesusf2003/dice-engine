//==============================
//	mapParser.h
//
//	reads quake .map text into a Map: entities with their keys and brushes,
//	faces in either texture format (standard or valve 220, see FaceTexture).
//	a syntax error stops everything with the line and column; things that
//	parse but aren't quake 1 (quake 2 surface flags, quake 3 patches and
//	brushDef) are skipped with a warning.
//	from trenchbroom's StandardMapParser, without the quake 2/3 and hexen 2 formats
//==============================

#ifndef __MAP_PARSER_H__
#define __MAP_PARSER_H__

#include <string>

class Map;
class MapReport;

struct MapParseError
{
	int			line, column;
	std::string	message;
};

namespace MapParser
{
	// fills map.entities and map.format, throws MapParseError
	void	Parse(const std::string &text, Map &map, MapReport &report);
}

#endif
