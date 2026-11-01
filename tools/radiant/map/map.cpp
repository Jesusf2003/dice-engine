//==============================
//	map.cpp
//==============================

#include "map/map.h"
#include "map/mapParser.h"
#include "map/mapReport.h"
#include "map/mapValidator.h"
#include "core/common/log.h"
#include "core/common/preferences.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

static unsigned s_revision;	// before g_map: zero initialized, so Map() can use it
Map g_map;

const char *MapFormatName(MapFormat format)
{
	switch (format)
	{
	case MAPFORMAT_STANDARD:	return "Quake standard";
	case MAPFORMAT_VALVE220:	return "Valve 220";
	case MAPFORMAT_MIXED:		return "mixed (standard + Valve 220)";
	default:					return "unknown (no faces)";
	}
}

Map::Map()
{
	New();
}

void Map::New()
{
	filename.clear();
	format = MAPFORMAT_UNKNOWN;
	stats = mapStats_t();
	entities.clear();
	entities.emplace_back();
	entities.back().SetKey("classname", "worldspawn");
	revision = ++s_revision;
}

Entity *Map::World()
{
	return entities.empty() ? nullptr : &entities[0];
}

std::string Map::Name() const
{
	return filename.empty() ? "unnamed" : fs::u8path(filename).filename().u8string();
}

/*
==================
SourceLine

the text of a line, tabs turned into spaces so a caret under it lines up with
the column (which counts characters)
==================
*/
static std::string SourceLine(const std::string &text, int line)
{
	size_t start = 0;
	for (int l = 1; l < line && start != std::string::npos; l++)
	{
		start = text.find('\n', start);
		if (start != std::string::npos)
			start++;
	}
	if (start == std::string::npos || start > text.size())
		return std::string();

	size_t end = text.find('\n', start);
	std::string s = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
	if (!s.empty() && s.back() == '\r')
		s.pop_back();
	std::replace(s.begin(), s.end(), '\t', ' ');
	return s;
}

static double MsSince(std::chrono::steady_clock::time_point t)
{
	return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count();
}

/*
==================
Map::Load

read, parse, validate, report. a syntax error leaves the current map alone;
anything the validator finds is reported but the map still loads, so it can
be fixed in the editor (trenchbroom does the same)
==================
*/
bool Map::Load(const std::string &path)
{
	const consoleAdvancedPrefs_t &adv = g_prefs.consoleAdvanced;
	fs::path fsPath = fs::u8path(path);
	std::string shortName = fsPath.filename().u8string();

	Log::Info("---- Loading map %s ----", path.c_str());

	std::string text;
	{
		std::ifstream f(fsPath, std::ios::binary);
		if (!f)
		{
			Log::Error("Couldn't open %s", path.c_str());
			return false;
		}
		text.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	}
	int lines = (int)std::count(text.begin(), text.end(), '\n') + (text.empty() || text.back() == '\n' ? 0 : 1);
	Log::Verbose("%s: %zu bytes, %d lines", shortName.c_str(), text.size(), lines);

	MapReport report(shortName, adv.warningLimit);
	Map loaded;
	loaded.entities.clear();
	loaded.filename = path;

	auto t0 = std::chrono::steady_clock::now();
	try
	{
		MapParser::Parse(text, loaded, report);
	}
	catch (const MapParseError &e)
	{
		Log::Error("%s:%d:%d: %s", shortName.c_str(), e.line, e.column, e.message.c_str());
		std::string src = SourceLine(text, e.line);
		if (!src.empty())
		{
			Log::Error("    %s", src.c_str());
			Log::Error("    %*s^", std::max(e.column - 1, 0), "");
		}
		Log::Error("Map not loaded, %s stays open", Name().c_str());
		return false;
	}
	double parseMs = MsSince(t0);

	auto t1 = std::chrono::steady_clock::now();
	MapValidator::Validate(loaded, report, fsPath.parent_path().u8string());
	double validateMs = MsSince(t1);
	report.Finish();

	// the report
	const mapStats_t &s = loaded.stats;
	Log::Info("%s loaded", shortName.c_str());
	Log::Info("  format:    %s", MapFormatName(loaded.format));
	Log::Info("  entities:  %zu (worldspawn, %d brush, %d point)", loaded.entities.size(), s.brushEntities, s.pointEntities);
	Log::Info("  brushes:   %d (%d world, %d in entities)%s", s.worldBrushes + s.entityBrushes, s.worldBrushes, s.entityBrushes,
		adv.validateGeometry ? "" : ", geometry not checked");
	if (s.invalidBrushes)
		Log::Info("             %d invalid", s.invalidBrushes);
	Log::Info("  faces:     %d", s.faces);
	if (adv.validateTextures)
		Log::Info("  textures:  %d used, %d missing", s.textures, s.missingTextures);
	else
		Log::Info("  textures:  %d used, not checked against the wads", s.textures);
	if (s.hasBounds)
		Log::Info("  bounds:    (%g %g %g) - (%g %g %g)", s.mins.x + 0.0, s.mins.y + 0.0, s.mins.z + 0.0,	// + 0.0: no "-0"
			s.maxs.x + 0.0, s.maxs.y + 0.0, s.maxs.z + 0.0);
	if (adv.timings)
		Log::Info("  time:      parse %.2f ms, validation %.2f ms", parseMs, validateMs);
	Log::Info("  result:    %d errors, %d warnings", report.Errors(), report.Warnings());

	*this = std::move(loaded);
	revision = ++s_revision;
	return true;
}

const Entity *Map::FindEntity(const std::string &classname) const
{
	for (const Entity &e : entities)
		if (e.Classname() == classname)
			return &e;
	return nullptr;
}

/*
==================
RayBox

slab test, the distance where the ray enters the box. false when it misses
or starts inside
==================
*/
static bool RayBox(const dvec3 &start, const dvec3 &dir, const dvec3 &mins, const dvec3 &maxs, double &dist)
{
	double enter = -DBL_MAX, leave = DBL_MAX;

	for (int i = 0; i < 3; i++)
	{
		if (fabs(dir[i]) < 1e-12)
		{
			if (start[i] < mins[i] || start[i] > maxs[i])
				return false;
			continue;
		}
		double t0 = (mins[i] - start[i]) / dir[i];
		double t1 = (maxs[i] - start[i]) / dir[i];
		if (t0 > t1)
			std::swap(t0, t1);
		enter = std::max(enter, t0);
		leave = std::min(leave, t1);
		if (enter > leave)
			return false;
	}
	if (enter < 0)
		return false;
	dist = enter;
	return true;
}

bool Map::RayCast(const dvec3 &start, const dvec3 &dir, double &dist) const
{
	bool hit = false;
	double best = DBL_MAX, d;
	dvec3 mins, maxs;

	for (const Entity &e : entities)
	{
		for (const Brush &b : e.brushes)
		{
			if (b.RayCast(start, dir, d) && d < best)
			{
				best = d;
				hit = true;
			}
		}
		if (e.PointBounds(mins, maxs) && RayBox(start, dir, mins, maxs, d) && d < best)
		{
			best = d;
			hit = true;
		}
	}
	if (hit)
		dist = best;
	return hit;
}
