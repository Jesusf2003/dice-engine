//==============================
//	entityDefs.h
//
//	the entity classes the game knows, read from its FGD (map/fgdParser):
//	point and solid classes with their properties, base classes they inherit
//	from, color and size. radiant's own dialect adds the "wadlist" property
//	type, worldspawn's wad entry lists the wads the editor loads (see
//	bin/ninjawars/ninjawars_radiant.fgd)
//==============================

#ifndef __ENTITY_DEFS_H__
#define __ENTITY_DEFS_H__

#include "mathlib.h"

#include <string>
#include <vector>

struct PropertyChoice
{
	std::string	value;			// choices / flags value, wadlist path
	std::string	description;
	bool		flagOn = false;	// flags: set by default
};

struct EntityProperty
{
	std::string	name;
	std::string	type;			// lowercase as written: string, integer, choices, flags, color255, wadlist...
	std::string	description;
	std::string	defaultValue;
	bool		hasDefault = false;
	std::vector<PropertyChoice>	choices;	// choices, flags, wadlist
	int			line = 0;
};

struct EntityClass
{
	enum Type { BASE, POINT, SOLID };

	Type		type = POINT;
	std::string	name, description;
	std::vector<std::string>	bases;
	bool		hasColor = false;
	vec3		color;			// 0-255
	bool		hasSize = false;
	dvec3		mins, maxs;
	std::vector<EntityProperty>	properties;		// its own, the bases' aren't copied in
	int			file = 0;		// index into EntityDefs::files
	int			line = 0;
};

class EntityDefs
{
public:
	std::vector<std::string>	files;		// in load order, a later file's classes replace earlier ones
	std::vector<EntityClass>	classes;

	// replaces everything, reports to the console. a file that doesn't parse is
	// left out; false when none did
	bool	Load(const std::vector<std::string> &paths);
	const EntityClass		*Find(const std::string &name) const;
	// its own first, then the bases in order (and theirs)
	const EntityProperty	*FindProperty(const EntityClass &cls, const std::string &key) const;

	// worldspawn's wadlist: every wad path, and the default a new map gets
	std::vector<std::string>	WadList() const;
	std::string					DefaultWads() const;

private:
	const EntityProperty	*FindProperty(const EntityClass &cls, const std::string &key, int depth) const;
	const EntityProperty	*WadProperty() const;
};

extern EntityDefs g_entityDefs;

#endif
