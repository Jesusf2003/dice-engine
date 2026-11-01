//==============================
//	entity.cpp
//==============================

#include "map/entity.h"
#include "map/entityDefs.h"

#include <cstdlib>

const std::string &Entity::ValueForKey(const std::string &key) const
{
	static const std::string empty;

	for (const keyValue_t &kv : keys)
		if (kv.first == key)
			return kv.second;
	return empty;
}

bool Entity::HasKey(const std::string &key) const
{
	for (const keyValue_t &kv : keys)
		if (kv.first == key)
			return true;
	return false;
}

void Entity::SetKey(const std::string &key, const std::string &value)
{
	for (keyValue_t &kv : keys)
	{
		if (kv.first == key)
		{
			kv.second = value;
			return;
		}
	}
	keys.emplace_back(key, value);
}

// up to n numbers separated by spaces, missing ones stay 0
static void ParseNumbers(const std::string &s, double *out, int n)
{
	const char *p = s.c_str();
	for (int i = 0; i < n; i++)
	{
		char *end;
		double v = strtod(p, &end);
		if (end == p)
			return;
		out[i] = v;
		p = end;
	}
}

dvec3 Entity::Origin() const
{
	dvec3 o;
	ParseNumbers(ValueForKey("origin"), &o.x, 3);
	return o;
}

/*
==================
Entity::Angles

quake's "angle" is the yaw, with -1 straight up and -2 straight down
==================
*/
void Entity::Angles(float &yaw, float &pitch) const
{
	yaw = pitch = 0;

	if (HasKey("angles"))
	{
		double a[3] = { 0, 0, 0 };	// pitch yaw roll
		ParseNumbers(ValueForKey("angles"), a, 3);
		pitch = (float)a[0];
		yaw = (float)a[1];
		return;
	}

	float a = (float)atof(ValueForKey("angle").c_str());
	if (a == -1)
		pitch = 90;
	else if (a == -2)
		pitch = -90;
	else
		yaw = a;
}

// color() and size() can come from a base class, the first one found wins
static const EntityClass *FindWith(const EntityClass *cls, bool EntityClass::*has, int depth = 0)
{
	if (!cls || depth > 32)
		return nullptr;
	if (cls->*has)
		return cls;
	for (const std::string &b : cls->bases)
		if (const EntityClass *found = FindWith(g_entityDefs.Find(b), has, depth + 1))
			return found;
	return nullptr;
}

bool Entity::PointBounds(dvec3 &mins, dvec3 &maxs) const
{
	if (IsWorldspawn() || !brushes.empty())
		return false;

	dvec3 o = Origin();
	const EntityClass *sized = FindWith(g_entityDefs.Find(Classname()), &EntityClass::hasSize);
	if (sized)
	{
		mins = o + sized->mins;
		maxs = o + sized->maxs;
	}
	else
	{
		mins = o - dvec3(8, 8, 8);		// quake's default for a class without a size
		maxs = o + dvec3(8, 8, 8);
	}
	return true;
}

vec3 Entity::Color() const
{
	const EntityClass *colored = FindWith(g_entityDefs.Find(Classname()), &EntityClass::hasColor);
	if (!colored)
		return vec3(0.6f, 0.6f, 0.6f);
	return colored->color / 255.0f;
}
