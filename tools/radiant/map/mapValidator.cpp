//==============================
//	mapValidator.cpp
//==============================

#include "map/mapValidator.h"
#include "map/map.h"
#include "map/mapReport.h"
#include "map/entityDefs.h"
#include "map/grid.h"
#include "core/common/log.h"
#include "core/common/preferences.h"
#include "renderer/texture.h"
#include "renderer/textures.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>

namespace fs = std::filesystem;

#define MAX_VALUE_LENGTH	1023	// the engine's entity parser token (qcc_token[1024])
#define MAX_TEXTURE_NAME	15		// WAD2/WAD3 lump names, 16 with the terminator

static std::string Lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s;
}

static std::string Trim(const std::string &s)
{
	size_t b = s.find_first_not_of(" \t");
	size_t e = s.find_last_not_of(" \t");
	return b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
}

static std::string Vec(const dvec3 &v)
{
	char buf[96];
	snprintf(buf, sizeof(buf), "(%g %g %g)", v.x + 0.0, v.y + 0.0, v.z + 0.0);	// + 0.0: no "-0"
	return buf;
}

/*
==============================================================================

	ENTITIES

==============================================================================
*/

static void ValidateEntities(Map &map, MapReport &report)
{
	mapStats_t &stats = map.stats;

	if (map.entities.empty())
	{
		report.Error("worldspawn", 0, "the file has no entities, not even worldspawn");
		return;
	}

	for (size_t i = 0; i < map.entities.size(); i++)
	{
		const Entity &e = map.entities[i];
		const std::string &cls = e.Classname();

		if (cls.empty())
			report.Error("missing classname", e.line, "entity %zu has no classname", i);
		else if (i == 0 && cls != "worldspawn")
			report.Error("worldspawn", e.line, "the first entity is \"%s\", it has to be worldspawn", cls.c_str());
		else if (i > 0 && cls == "worldspawn")
			report.Error("worldspawn", e.line, "entity %zu is a second worldspawn", i);

		for (const Entity::keyValue_t &kv : e.keys)
		{
			if (kv.first.size() > MAX_VALUE_LENGTH || kv.second.size() > MAX_VALUE_LENGTH)
				report.Error("value too long", e.line, "entity %zu (%s): \"%s\" is %zu characters, the engine's entity parser stops at %d",
					i, cls.c_str(), kv.first.c_str(), std::max(kv.first.size(), kv.second.size()), MAX_VALUE_LENGTH);
		}

		if (e.HasKey("origin"))
		{
			const char *s = e.ValueForKey("origin").c_str();
			char *end;
			int n = 0;
			for (; n < 3; n++)
			{
				strtod(s, &end);
				if (end == s)
					break;
				s = end;
			}
			if (n < 3 || *Trim(s).c_str())
				report.Warning("bad origin", e.line, "entity %zu (%s): origin \"%s\" isn't three numbers",
					i, cls.c_str(), e.ValueForKey("origin").c_str());
		}

		if (i > 0)
		{
			if (e.brushes.empty())
			{
				stats.pointEntities++;
				// TODO: ask the FGD instead of guessing from the name, once it's loaded
				if (!cls.compare(0, 5, "func_") || !cls.compare(0, 8, "trigger_"))
					report.Warning("brush entity without brushes", e.line, "entity %zu (%s) has no brushes, but its class looks like a brush entity",
						i, cls.c_str());
			}
			else
				stats.brushEntities++;
		}

		Log::Verbose("  entity %zu (line %d): %s, %zu keys, %zu brushes",
			i, e.line, cls.empty() ? "<no classname>" : cls.c_str(), e.keys.size(), e.brushes.size());
	}
}

/*
==============================================================================

	WADS

	worldspawn's "wad": ';' separated paths. the engine only uses each base name
	(the textures come from the bsp) and qbsp looks them up itself, so the value
	only has to be short and portable:
		./name.wad, ../dir/name.wad, dir/name.wad, name.wad	relative to the game folder
		C:/.../name.wad										absolute: ties the map to one
															machine and can grow past what
															the engine reads (J.A.C.K.'s did)
	the entity definitions' wadlist is what the editor has loaded, a wad
	outside it has no textures in the browser
==============================================================================
*/

static const char *WadPathKind(const std::string &entry, bool absolute)
{
	if (absolute)
		return "absolute";
	if (!entry.compare(0, 2, "./") || !entry.compare(0, 2, ".\\") || !entry.compare(0, 3, "../") || !entry.compare(0, 3, "..\\"))
		return "relative";
	if (entry.find_first_of("/\\") != std::string::npos)
		return "relative, subfolder";
	return "name only";
}

// what the wadlist and the map's entries are compared by: the file they end up at
// through the game's folders (../ninjawars/tools.wad and ./tools.wad are the same one)
static std::string WadKey(const fs::path &relative)
{
	std::string found = Textures::FindWad(relative.u8string());
	if (!found.empty())
		return Lower(fs::u8path(found).lexically_normal().generic_u8string());

	fs::path first = Textures::searchPaths.empty() ? fs::path() : fs::u8path(Textures::searchPaths.front());
	return Lower((first / relative).lexically_normal().generic_u8string());
}

static void ValidateWads(Map &map, MapReport &report, const std::string &mapDir)
{
	if (map.entities.empty() || !map.entities[0].IsWorldspawn())
		return;

	const Entity &world = map.entities[0];
	const std::string &wadKey = world.ValueForKey("wad");

	if (wadKey.empty())
	{
		report.Warning("no wad", world.line, "worldspawn has no \"wad\" key, qbsp won't find any textures");
		return;
	}

	std::string folders;	// for the messages: the mod's and the game's
	for (const std::string &d : Textures::searchPaths)
		folders += (folders.empty() ? "" : " or ") + d;
	if (folders.empty())
		folders = "the game's folders (no game configuration)";

	std::set<std::string> fgdWads;
	for (const std::string &w : g_entityDefs.WadList())
		fgdWads.insert(WadKey(fs::u8path(w)));

	std::set<std::string> seen;
	std::string shortened;		// the value with every absolute path made relative
	bool anyAbsolute = false;
	size_t start = 0;

	while (start <= wadKey.size())
	{
		size_t semi = wadKey.find(';', start);
		std::string entry = Trim(wadKey.substr(start, semi == std::string::npos ? std::string::npos : semi - start));
		start = (semi == std::string::npos) ? wadKey.size() + 1 : semi + 1;
		if (entry.empty())
			continue;

		fs::path p = fs::u8path(entry);
		bool absolute = p.is_absolute() || p.has_root_name() || entry[0] == '/' || entry[0] == '\\';

		// relative to the game's folders: as written, or worked out from an absolute path inside one
		fs::path relative = p;
		std::string suggestion = entry;
		if (absolute)
		{
			anyAbsolute = true;
			relative = p.filename();
			for (const std::string &d : Textures::searchPaths)
			{
				fs::path rel = p.lexically_normal().lexically_relative(fs::u8path(d));
				if (!rel.empty() && rel.generic_u8string().compare(0, 2, "..") != 0)
				{
					relative = rel;
					break;
				}
			}
			suggestion = "./" + relative.generic_u8string();
			report.Warning("absolute wad path", world.line, "wad \"%s\" is an absolute path, store it as \"%s\" (relative to the game folder)",
				entry.c_str(), suggestion.c_str());
		}
		shortened += (shortened.empty() ? "" : ";") + suggestion;

		std::string key = WadKey(relative);
		if (!seen.insert(key).second)
			report.Warning("duplicate wad", world.line, "wad \"%s\" is listed twice", entry.c_str());

		// where it is: the mod's folder, the game's (that's what ./ means), then next to the map
		std::string found = Textures::FindWad(absolute ? entry : relative.u8string());
		if (found.empty() && !absolute)
		{
			std::error_code ec;
			fs::path nextToMap = (fs::u8path(mapDir) / relative).lexically_normal();
			if (fs::is_regular_file(nextToMap, ec))
				found = nextToMap.u8string();
		}

		if (!found.empty())
			Log::Verbose("  wad %s (%s) -> %s", entry.c_str(), WadPathKind(entry, absolute), found.c_str());
		else
			report.Warning("missing wad", world.line, "wad \"%s\" isn't in %s or next to the map", entry.c_str(), folders.c_str());

		if (!fgdWads.empty() && !fgdWads.count(key))
			report.Warning("wad not in the definitions", world.line, "wad \"%s\" isn't in the game's wadlist (entity definitions), its textures aren't loaded",
				entry.c_str());
	}

	if (anyAbsolute)
		report.Warning("wad value length", world.line, "the wad value is %zu characters, %zu with relative paths: \"%s\"",
			wadKey.size(), shortened.size(), shortened.c_str());
}

/*
==============================================================================

	BRUSHES

==============================================================================
*/

static void ReportBrushProblem(MapReport &report, const Brush &b, const BrushProblem &p, const char *where)
{
	const BrushFace *f = (p.face >= 0) ? &b.faces[p.face] : nullptr;
	int line = f ? f->line : b.line;

	switch (p.kind)
	{
	case BrushProblem::DEGENERATE_FACE:
		report.Warning("degenerate face", line, "%s face %d: its three points are in a line, the face is ignored", where, p.face);
		break;
	case BrushProblem::DUPLICATE_PLANE:
		report.Warning("duplicate plane", line, "%s face %d: same plane as face %d, ignored", where, p.face, p.other);
		break;
	case BrushProblem::REDUNDANT_FACE:
		report.Warning("redundant face", line, "%s face %d (%s): doesn't touch the brush, qbsp drops it", where, p.face, f->tex.name.c_str());
		break;
	case BrushProblem::TOO_FEW_FACES:
		report.Error("invalid brush", line, "%s: fewer than 4 usable planes, it can't enclose anything", where);
		break;
	case BrushProblem::NO_VOLUME:
		report.Error("invalid brush", line, "%s: its planes don't enclose any space", where);
		break;
	case BrushProblem::UNBOUNDED:
		report.Error("invalid brush", line, "%s: its planes leave it open, it isn't a closed solid", where);
		break;
	case BrushProblem::OUT_OF_BOUNDS:
		report.Warning("out of bounds", line, "%s reaches past +-%g", where, MAX_WORLD_COORD);
		break;
	}
}

static void ValidateFaceTexture(MapReport &report, const BrushFace &f, int index, const char *where)
{
	const FaceTexture &t = f.tex;

	if (t.name.size() > MAX_TEXTURE_NAME)
		report.Warning("texture name too long", f.line, "%s face %d: \"%s\" is longer than a wad lump name (%d)", where, index, t.name.c_str(), MAX_TEXTURE_NAME);

	if (t.scale[0] == 0 || t.scale[1] == 0)
		report.Warning("zero texture scale", f.line, "%s face %d: texture scale %g %g, qbsp treats 0 as 1", where, index, t.scale[0], t.scale[1]);

	if (t.valve)
	{
		for (int a = 0; a < 2; a++)
		{
			if (length(t.axis[a]) < 1e-6)
				report.Warning("bad texture axis", f.line, "%s face %d: the %c texture axis is zero", where, index, "UV"[a]);
		}
		// an axis along the normal projects the whole texture onto a line
		if (f.planeValid && length(cross(t.axis[0], t.axis[1])) > 1e-6)
		{
			dvec3 n = cross(t.axis[0], t.axis[1]);
			if (fabs(dot(n / length(n), f.plane.normal)) < 1e-3)
				report.Warning("bad texture axis", f.line, "%s face %d: the texture axes lie across the face, it'll be smeared", where, index);
		}
	}
}

static void ValidateBrushes(Map &map, MapReport &report)
{
	mapStats_t &stats = map.stats;
	bool geometry = g_prefs.consoleAdvanced.validateGeometry;
	bool debug = Log::Enabled(LOG_DEBUG);
	std::vector<BrushProblem> problems;
	char where[64];

	stats.mins = dvec3(DBL_MAX, DBL_MAX, DBL_MAX);
	stats.maxs = dvec3(-DBL_MAX, -DBL_MAX, -DBL_MAX);

	for (size_t ei = 0; ei < map.entities.size(); ei++)
	{
		Entity &e = map.entities[ei];

		if (debug && !e.brushes.empty())
			Log::Debug("  entity %zu (%s) brushes:", ei, e.Classname().c_str());

		for (size_t bi = 0; bi < e.brushes.size(); bi++)
		{
			Brush &b = e.brushes[bi];
			snprintf(where, sizeof(where), "entity %zu brush %zu", ei, bi);

			if (ei == 0)
				stats.worldBrushes++;
			else
				stats.entityBrushes++;
			stats.faces += (int)b.faces.size();

			if (geometry)
			{
				problems.clear();
				if (!b.BuildGeometry(problems))
					stats.invalidBrushes++;
				for (const BrushProblem &p : problems)
					ReportBrushProblem(report, b, p, where);

				if (b.valid)
				{
					for (int k = 0; k < 3; k++)
					{
						stats.mins[k] = std::min(stats.mins[k], b.mins[k]);
						stats.maxs[k] = std::max(stats.maxs[k], b.maxs[k]);
					}
					stats.hasBounds = true;
				}
			}

			for (size_t fi = 0; fi < b.faces.size(); fi++)
				ValidateFaceTexture(report, b.faces[fi], (int)fi, where);

			if (!debug)
				continue;

			if (geometry)
				Log::Debug("    brush %zu (line %d): %zu faces, %s, bounds %s - %s", bi, b.line, b.faces.size(),
					b.valid ? "valid" : "INVALID", Vec(b.mins).c_str(), Vec(b.maxs).c_str());
			else
				Log::Debug("    brush %zu (line %d): %zu faces", bi, b.line, b.faces.size());

			for (size_t fi = 0; fi < b.faces.size(); fi++)
			{
				const BrushFace &f = b.faces[fi];
				if (geometry && f.planeValid)
					Log::Debug("      face %zu (line %d) %s: normal %s dist %g, %zu points", fi, f.line, f.tex.name.c_str(),
						Vec(f.plane.normal).c_str(), f.plane.dist, f.winding.size());
				else
					Log::Debug("      face %zu (line %d) %s", fi, f.line, f.tex.name.c_str());
			}
		}
	}
}

/*
==============================================================================

	TEXTURES

	against everything the texture browser loaded: the entity definitions'
	wadlist (every wad in the game folder without one)
==============================================================================
*/

static void ValidateTextures(Map &map, MapReport &report)
{
	std::map<std::string, int> used;		// lowercase name -> faces

	for (const Entity &e : map.entities)
		for (const Brush &b : e.brushes)
			for (const BrushFace &f : b.faces)
				used[Lower(f.tex.name)]++;

	map.stats.textures = (int)used.size();
	if (!g_prefs.consoleAdvanced.validateTextures)
		return;

	// Textures::ForName makes stand-ins for unknown names, those don't count
	std::set<const Texture *> broken;
	for (Texture *t = Textures::group_unknown.first; t; t = t->next)
		broken.insert(t);

	for (const auto &u : used)
	{
		auto it = Textures::texMap.find(u.first);
		if (it != Textures::texMap.end() && it->second && !broken.count(it->second))
			continue;

		map.stats.missingTextures++;
		report.Warning("missing texture", 0, "texture \"%s\" (%d faces) isn't in any loaded wad (the wadlist of the entity definitions)",
			u.first.c_str(), u.second);
	}
}

/*
==============================================================================

	TARGETS

==============================================================================
*/

static void ValidateTargets(Map &map, MapReport &report)
{
	std::set<std::string> names;
	for (const Entity &e : map.entities)
		if (e.HasKey("targetname"))
			names.insert(e.ValueForKey("targetname"));

	static const char *targetKeys[] = { "target", "killtarget" };

	for (size_t i = 0; i < map.entities.size(); i++)
	{
		const Entity &e = map.entities[i];
		for (const char *key : targetKeys)
		{
			const std::string &v = e.ValueForKey(key);
			if (!v.empty() && !names.count(v))
				report.Warning("missing target", e.line, "entity %zu (%s): %s \"%s\", no entity has that targetname",
					i, e.Classname().c_str(), key, v.c_str());
		}
	}
}

/*
==================
ValidateFormat

valve 220 faces need worldspawn's "mapversion" "220" so qbsp reads the axes
==================
*/
static void ValidateFormat(Map &map, MapReport &report)
{
	if (map.entities.empty())
		return;

	const Entity &world = map.entities[0];
	bool says220 = world.ValueForKey("mapversion") == "220";

	switch (map.format)
	{
	case MAPFORMAT_MIXED:
		report.Warning("mixed format", 0, "faces in both the standard and the Valve 220 texture format");
		break;
	case MAPFORMAT_STANDARD:
		if (says220)
			report.Warning("mapversion", world.line, "worldspawn says mapversion 220 but the faces are in the standard format");
		break;
	case MAPFORMAT_VALVE220:
		if (!says220)
			report.Warning("mapversion", world.line, "Valve 220 faces without \"mapversion\" \"220\" in worldspawn");
		break;
	default:
		break;
	}
}

void MapValidator::Validate(Map &map, MapReport &report, const std::string &mapDir)
{
	const consoleAdvancedPrefs_t &adv = g_prefs.consoleAdvanced;

	map.stats = mapStats_t();

	ValidateEntities(map, report);
	ValidateFormat(map, report);
	ValidateWads(map, report, mapDir);
	ValidateBrushes(map, report);
	ValidateTextures(map, report);
	if (adv.validateTargets)
		ValidateTargets(map, report);
}
