//==============================
//	entity.h
//
//	a map entity: key/value pairs in file order plus its brushes. worldspawn is
//	the first one and holds the world's brushes; any other entity with
//	brushes is a brush entity (func_door...), without them a point entity
//==============================

#ifndef __ENTITY_H__
#define __ENTITY_H__

#include "map/brush.h"

#include <string>
#include <utility>
#include <vector>

class Entity
{
public:
	typedef std::pair<std::string, std::string> keyValue_t;

	std::vector<keyValue_t>	keys;
	std::vector<Brush>		brushes;
	int		line = 0;			// the entity's opening brace in the .map file

	const std::string	&ValueForKey(const std::string &key) const;	// "" when missing
	bool	HasKey(const std::string &key) const;
	void	SetKey(const std::string &key, const std::string &value);	// replaces, keeps the position
	inline const std::string &Classname() const		{ return ValueForKey("classname"); }
	inline bool IsWorldspawn() const				{ return Classname() == "worldspawn"; }

	// "origin" and "angle" / "angles" (pitch yaw roll) as quake reads them
	dvec3	Origin() const;
	void	Angles(float &yaw, float &pitch) const;

	// a point entity's box in the world: its class' size() around the origin, the
	// definitions' bases included. false for worldspawn and brush entities
	bool	PointBounds(dvec3 &mins, dvec3 &maxs) const;
	vec3	Color() const;		// the class' color() 0..1, gray for an unknown class
};

#endif
