//==============================
//	entityDefs.cpp
//==============================

#include "map/entityDefs.h"
#include "map/fgdParser.h"
#include "map/mapReport.h"
#include "core/common/log.h"
#include "core/common/preferences.h"
#include "renderer/textures.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>

namespace fs = std::filesystem;

EntityDefs g_entityDefs;

const EntityClass *EntityDefs::Find(const std::string &name) const
{
	// the last definition wins: a mod's FGD over its game's
	for (auto it = classes.rbegin(); it != classes.rend(); ++it)
		if (it->name == name)
			return &*it;
	return nullptr;
}

const EntityProperty *EntityDefs::FindProperty(const EntityClass &cls, const std::string &key) const
{
	return FindProperty(cls, key, 0);
}

const EntityProperty *EntityDefs::FindProperty(const EntityClass &cls, const std::string &key, int depth) const
{
	if (depth > 32)		// a base loop, ValidateDefs already reported it
		return nullptr;

	for (auto it = cls.properties.rbegin(); it != cls.properties.rend(); ++it)
		if (it->name == key)
			return &*it;

	for (const std::string &b : cls.bases)
	{
		const EntityClass *base = Find(b);
		if (!base)
			continue;
		if (const EntityProperty *p = FindProperty(*base, key, depth + 1))
			return p;
	}
	return nullptr;
}

const EntityProperty *EntityDefs::WadProperty() const
{
	const EntityClass *world = Find("worldspawn");
	if (!world)
		return nullptr;
	const EntityProperty *p = FindProperty(*world, "wad");
	return (p && p->type == "wadlist") ? p : nullptr;
}

std::vector<std::string> EntityDefs::WadList() const
{
	std::vector<std::string> wads;
	if (const EntityProperty *p = WadProperty())
		for (const PropertyChoice &c : p->choices)
			wads.push_back(c.value);
	return wads;
}

std::string EntityDefs::DefaultWads() const
{
	const EntityClass *world = Find("worldspawn");
	const EntityProperty *p = world ? FindProperty(*world, "wad") : nullptr;
	return (p && p->hasDefault) ? p->defaultValue : std::string();
}

/*
==============================================================================

	VALIDATION

==============================================================================
*/

static bool IsAbsolutePath(const std::string &s)
{
	fs::path p = fs::u8path(s);
	return p.is_absolute() || p.has_root_name() || (!s.empty() && (s[0] == '/' || s[0] == '\\'));
}

// the classes that came from files[file], looked up against all of them
static void ValidateDefs(const EntityDefs &defs, int file, MapReport &report)
{
	std::map<std::string, int> seen;		// name -> line, in this file

	for (const EntityClass &cls : defs.classes)
	{
		if (cls.file != file)
			continue;

		if (seen.count(cls.name))
			report.Warning("duplicate class", cls.line, "%s is defined again (first at line %d), the last one is used",
				cls.name.c_str(), seen[cls.name]);
		else
		{
			seen[cls.name] = cls.line;
			for (const EntityClass &earlier : defs.classes)
			{
				if (earlier.file < file && earlier.name == cls.name)
				{
					Log::Verbose("  %s replaces %s from %s", cls.name.c_str(), cls.name.c_str(),
						fs::u8path(defs.files[earlier.file]).filename().u8string().c_str());
					break;
				}
			}
		}

		for (const std::string &b : cls.bases)
		{
			const EntityClass *base = defs.Find(b);
			if (!base)
				report.Warning("missing base class", cls.line, "%s: base class %s isn't defined", cls.name.c_str(), b.c_str());
			else if (base == &cls)
				report.Error("base class loop", cls.line, "%s has itself as a base class", cls.name.c_str());
		}

		for (const EntityProperty &p : cls.properties)
		{
			if (p.type == "choices" && p.hasDefault && !p.choices.empty())
			{
				bool found = std::any_of(p.choices.begin(), p.choices.end(),
					[&](const PropertyChoice &c) { return c.value == p.defaultValue; });
				if (!found)
					report.Warning("default not a choice", p.line, "%s: %s defaults to %s, which isn't one of its choices",
						cls.name.c_str(), p.name.c_str(), p.defaultValue.c_str());
			}
			else if (p.type == "flags")
			{
				for (const PropertyChoice &c : p.choices)
				{
					long v = strtol(c.value.c_str(), nullptr, 10);
					if (v <= 0 || (v & (v - 1)))
						report.Warning("bad flag", p.line, "%s: %s flag %s isn't a power of two",
							cls.name.c_str(), p.name.c_str(), c.value.c_str());
				}
			}
			else if (p.type == "wadlist")
			{
				if (cls.name != "worldspawn" || p.name != "wad")
					report.Warning("wadlist", p.line, "%s: wadlist only means something as worldspawn's wad, %s is ignored",
						cls.name.c_str(), p.name.c_str());

				for (const PropertyChoice &c : p.choices)
				{
					if (IsAbsolutePath(c.value))
						report.Warning("absolute wad path", p.line, "wad \"%s\" is an absolute path, write it relative to the game folder (./%s)",
							c.value.c_str(), fs::u8path(c.value).filename().u8string().c_str());
					if (Textures::FindWad(c.value).empty())
						report.Warning("missing wad", p.line, "wad \"%s\" isn't in the game's folders", c.value.c_str());
				}
			}
		}
	}
}

static bool ReadFile(const std::string &path, std::string &text)
{
	std::ifstream f(fs::u8path(path), std::ios::binary);
	if (!f)
		return false;
	text.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
	return true;
}

/*
==================
EntityDefs::Load
==================
*/
bool EntityDefs::Load(const std::vector<std::string> &paths)
{
	*this = EntityDefs();
	if (paths.empty())
		return false;

	int errors = 0, warnings = 0;

	for (const std::string &path : paths)
	{
		std::string shortName = fs::u8path(path).filename().u8string();
		Log::Info("---- Loading entity definitions %s ----", path.c_str());

		std::string text;
		if (!ReadFile(path, text))
		{
			Log::Error("Couldn't open %s", path.c_str());
			errors++;
			continue;
		}

		MapReport report(shortName, g_prefs.consoleAdvanced.warningLimit);
		size_t first = classes.size();
		try
		{
			FgdParser::Parse(text, *this, report);
		}
		catch (const FgdParseError &e)
		{
			classes.resize(first);		// nothing half read
			Log::Error("%s:%d:%d: %s", shortName.c_str(), e.line, e.column, e.message.c_str());
			Log::Error("%s left out", shortName.c_str());
			errors++;
			continue;
		}

		int file = (int)files.size();
		files.push_back(path);
		for (size_t i = first; i < classes.size(); i++)
			classes[i].file = file;

		int counts[3] = {};
		for (size_t i = first; i < classes.size(); i++)
		{
			const EntityClass &cls = classes[i];
			counts[cls.type]++;

			static const char *types[] = { "base", "point", "solid" };
			std::string bases;
			for (const std::string &b : cls.bases)
				bases += (bases.empty() ? ", bases " : " ") + b;
			Log::Verbose("  %s %s (line %d): %zu properties%s", types[cls.type], cls.name.c_str(), cls.line,
				cls.properties.size(), bases.c_str());
		}
		Log::Info("%s: %zu classes (%d base, %d point, %d solid)", shortName.c_str(), classes.size() - first,
			counts[EntityClass::BASE], counts[EntityClass::POINT], counts[EntityClass::SOLID]);

		// what the parser found; the checks wait until all files are in, a base class can come from an earlier one
		report.Finish();
		warnings += report.Warnings();
	}

	// the checks, file by file so the lines point at the right one
	for (int f = 0; f < (int)files.size(); f++)
	{
		MapReport report(fs::u8path(files[f]).filename().u8string(), g_prefs.consoleAdvanced.warningLimit);
		ValidateDefs(*this, f, report);
		report.Finish();
		errors += report.Errors();
		warnings += report.Warnings();
	}

	if (files.empty())
	{
		Log::Error("No entity definitions loaded");
		return false;
	}

	if (!Find("worldspawn"))
	{
		Log::Warning("The entity definitions have no worldspawn class");
		warnings++;
	}

	std::vector<std::string> wads = WadList();
	Log::Info("Entity definitions: %zu files, %zu classes", files.size(), classes.size());
	if (!wads.empty())
		Log::Info("  wadlist:  %zu wads, new maps get \"%s\"", wads.size(), DefaultWads().c_str());
	Log::Info("  result:   %d errors, %d warnings", errors, warnings);
	return true;
}
