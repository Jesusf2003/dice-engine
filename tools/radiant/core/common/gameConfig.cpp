//==============================
//	gameConfig.cpp
//==============================

#include "core/common/gameConfig.h"
#include "core/common/log.h"

#include <SDL.h>
#include <imgui.h>
#include <imgui_internal.h>	// ImGuiSettingsHandler

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

std::vector<gameConfig_t>	GameConfigs::configs;
std::string					GameConfigs::activeName;

static gameConfig_t	s_active;		// Active(), with what it inherits
static bool			s_activeValid;

const gameConfig_t *GameConfigs::Active()
{
	if (!s_activeValid)
	{
		const gameConfig_t *c = Find(configs, activeName);
		if (!c || !CanBeActive(c->kind))
			return nullptr;
		s_active = Inherited(configs, *c);
		s_activeValid = true;
	}
	return &s_active;
}

void GameConfigs::Apply(const std::vector<gameConfig_t> &list, const std::string &active)
{
	configs = list;
	activeName = active;
	Changed();
}

void GameConfigs::Changed()
{
	s_activeValid = false;
	ImGui::MarkIniSettingsDirty();
}

gameConfig_t *GameConfigs::CompileOwner()
{
	for (gameConfig_t &c : configs)
	{
		if (c.name != activeName)
			continue;
		if (c.kind == CONFIG_GAME)
			return &c;
		if (c.kind == CONFIG_MOD)
		{
			const gameConfig_t *game = Parent(configs, c);
			return game ? &configs[game - configs.data()] : nullptr;
		}
	}
	return nullptr;
}

/*
==============================================================================

	PROFILES

==============================================================================
*/

const char *GameConfigs::KindName(int kind)
{
	switch (kind)
	{
	case CONFIG_ENGINE:	return "Engine";
	case CONFIG_MOD:	return "Mod";
	default:			return "Game";
	}
}

int GameConfigs::ParentKind(int kind)
{
	return kind == CONFIG_MOD ? CONFIG_GAME : kind == CONFIG_GAME ? CONFIG_ENGINE : -1;
}

bool GameConfigs::CanBeActive(int kind)
{
	return kind == CONFIG_GAME || kind == CONFIG_MOD;
}

const gameConfig_t *GameConfigs::Find(const std::vector<gameConfig_t> &list, const std::string &name)
{
	for (const gameConfig_t &c : list)
		if (c.name == name)
			return &c;
	return nullptr;
}

const gameConfig_t *GameConfigs::Parent(const std::vector<gameConfig_t> &list, const gameConfig_t &cfg)
{
	if (cfg.parent.empty())
		return nullptr;
	const gameConfig_t *p = Find(list, cfg.parent);
	return (p && p->kind == ParentKind(cfg.kind)) ? p : nullptr;
}

/*
==================
GameConfigs::Inherited

from the engine down: a value a profile sets replaces its parent's, the FGDs add
up, tools replace the ones of the same name. the maps folder is the profile's
own, a mod's maps aren't its game's; the compilation profiles are the game's
==================
*/
gameConfig_t GameConfigs::Inherited(const std::vector<gameConfig_t> &list, const gameConfig_t &cfg)
{
	std::vector<const gameConfig_t *> chain;	// cfg, its parent, ...
	for (const gameConfig_t *c = &cfg; c && chain.size() < CONFIG_KIND_COUNT; c = Parent(list, *c))
		chain.push_back(c);

	gameConfig_t out;
	for (auto it = chain.rbegin(); it != chain.rend(); ++it)
	{
		const gameConfig_t &c = **it;
		out.fgds.insert(out.fgds.end(), c.fgds.begin(), c.fgds.end());
		for (const compileTool_t &t : c.tools)
		{
			auto same = std::find_if(out.tools.begin(), out.tools.end(), [&](const compileTool_t &o) { return o.name == t.name; });
			if (same != out.tools.end())
				*same = t;
			else
				out.tools.push_back(t);
		}
		if (c.kind == CONFIG_GAME)
			out.compileProfiles = c.compileProfiles;
		for (auto field : { &gameConfig_t::executableDir, &gameConfig_t::executable, &gameConfig_t::launchParams,
				&gameConfig_t::gameDir, &gameConfig_t::modDir, &gameConfig_t::defaultPointClass, &gameConfig_t::defaultSolidClass })
		{
			if (!(c.*field).empty())
				out.*field = c.*field;
		}
	}
	out.name = cfg.name;
	out.kind = cfg.kind;
	out.parent = cfg.parent;
	out.mapsDir = cfg.mapsDir;
	return out;
}

/*
==============================================================================

	PATHS

==============================================================================
*/

std::string GameConfigs::EditorDir()
{
	static std::string dir;
	if (dir.empty())
	{
		char *base = SDL_GetBasePath();
		dir = fs::u8path(base ? base : ".").lexically_normal().u8string();
		SDL_free(base);
	}
	return dir;
}

std::string GameConfigs::Resolve(const std::string &path)
{
	if (path.empty())
		return path;
	fs::path p = fs::u8path(path);
	if (!p.is_absolute())
		p = fs::u8path(EditorDir()) / p;
	p = p.lexically_normal();
	if (!p.has_filename())		// no trailing separator
		p = p.parent_path();
	return p.u8string();
}

std::string GameConfigs::MakeRelative(const std::string &path)
{
	fs::path p = fs::u8path(path).lexically_normal();
	if (!p.is_absolute())
		return p.generic_u8string();

	fs::path rel = p.lexically_relative(fs::u8path(EditorDir()));
	if (rel.empty() || !rel.generic_u8string().compare(0, 2, ".."))
		return p.generic_u8string();	// outside the editor's folder: stays absolute
	return rel.generic_u8string();
}

std::vector<std::string> GameConfigs::SearchPaths(const gameConfig_t &cfg)
{
	std::vector<std::string> dirs;
	for (const std::string *d : { &cfg.modDir, &cfg.gameDir })
	{
		if (d->empty())
			continue;
		std::string r = Resolve(*d);
		if (std::find(dirs.begin(), dirs.end(), r) == dirs.end())
			dirs.push_back(r);
	}
	return dirs;
}

std::string GameConfigs::MapsDir(const gameConfig_t &cfg)
{
	if (!cfg.mapsDir.empty())
		return Resolve(cfg.mapsDir);
	std::vector<std::string> dirs = SearchPaths(cfg);
	return dirs.empty() ? EditorDir() : (fs::u8path(dirs.front()) / "maps").u8string();
}

/*
==================
FindProgram

the first of names (without extension) that's a file in dir, relative to the editor
==================
*/
static std::string FindProgram(const fs::path &dir, std::initializer_list<const char *> names)
{
	std::error_code ec;
	for (const char *n : names)
	{
#ifdef _WIN32
		fs::path p = dir / (std::string(n) + ".exe");
#else
		fs::path p = dir / n;
#endif
		if (fs::is_regular_file(p, ec))
			return GameConfigs::MakeRelative(p.u8string());
	}
	return std::string();
}

/*
==================
EngineProfile

the engine in dir, and the compilation tools next to it. empty name when there's no engine
==================
*/
static gameConfig_t EngineProfile(const std::string &dir)
{
	gameConfig_t e;
	e.kind = CONFIG_ENGINE;
	fs::path full = fs::u8path(GameConfigs::Resolve(dir));
	e.executable = FindProgram(full, { "fteqw64", "fteqw", "fteqw32" });
	if (e.executable.empty())
		return e;

	e.name = "fteqw";
	e.executableDir = dir;
	e.launchParams = DEFAULT_LAUNCH_PARAMS;
	e.tools = GameConfigs::DetectTools(dir);
	return e;
}

const compileTool_t *GameConfigs::FindTool(const gameConfig_t &cfg, const std::string &name)
{
	for (const compileTool_t &t : cfg.tools)
		if (t.name == name)
			return &t;
	return nullptr;
}

std::vector<compileTool_t> GameConfigs::DetectTools(const std::string &dir)
{
	std::vector<compileTool_t> tools;
	fs::path full = fs::u8path(Resolve(dir.empty() ? std::string(".") : dir));
	for (const char *name : { "qbsp", "vis", "light", "vmap" })
	{
		std::string path = FindProgram(full, { name });
		if (!path.empty())
			tools.push_back({ name, path });
	}
	return tools;
}

/*
==================
GameConfigs::DefaultCompileProfiles

trenchbroom's come with the game config; these are made from the tools there
are: ericw-tools' qbsp, vis and light, or vmap's three stages. the bsp is made
next to the map and copied to the game's (the mod's) maps folder
==================
*/
static compileTask_t MakeTask(int type, const std::string &a, const std::string &b = std::string(), bool enabled = true)
{
	compileTask_t t;
	t.type = type;
	t.a = a;
	t.b = b;
	t.enabled = enabled;
	return t;
}

std::vector<compileProfile_t> GameConfigs::DefaultCompileProfiles(const std::vector<compileTool_t> &tools)
{
	auto has = [&](const char *name) {
		return std::any_of(tools.begin(), tools.end(), [&](const compileTool_t &t) { return t.name == name; });
	};
	const std::string bsp = "\"${MAP_BASE_NAME}.bsp\"";
	const std::string map = "\"${MAP_FULL_NAME}\"";

	std::vector<compileProfile_t> out;
	if (has("qbsp"))
	{
		for (int fast = 0; fast < 2; fast++)
		{
			compileProfile_t p;
			p.name = fast ? "Fast (no vis)" : "Full compile";
			p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${qbsp}", map));
			if (has("vis") && !fast)
				p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${vis}", "-threads ${CPU_COUNT} " + bsp));
			if (has("light"))
				p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${light}", std::string("-threads ${CPU_COUNT} ") + (fast ? "" : "-extra4 ") + bsp));
			p.tasks.push_back(MakeTask(TASK_COPY_FILES, "${MAP_BASE_NAME}.bsp", "${MAPS_DIR_PATH}"));
			p.tasks.push_back(MakeTask(TASK_COPY_FILES, "${MAP_BASE_NAME}.lit", "${MAPS_DIR_PATH}"));
			p.tasks.push_back(MakeTask(TASK_LAUNCH_ENGINE, std::string(), std::string(), fast != 0));
			out.push_back(p);
		}
	}

	if (has("vmap"))
	{
		compileProfile_t p;
		p.name = "vmap";
		p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${vmap}", "-threads ${CPU_COUNT} " + map));
		p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${vmap}", "-vis -threads ${CPU_COUNT} " + map));
		p.tasks.push_back(MakeTask(TASK_RUN_TOOL, "${vmap}", "-light -threads ${CPU_COUNT} " + map));
		p.tasks.push_back(MakeTask(TASK_COPY_FILES, "${MAP_BASE_NAME}.bsp", "${MAPS_DIR_PATH}"));
		p.tasks.push_back(MakeTask(TASK_LAUNCH_ENGINE, std::string(), std::string(), false));
		out.push_back(p);
	}
	return out;
}

/*
==================
GameConfigs::Detect

every folder next to the editor with an FGD in it is a game, on the engine next
to the editor when there's one. a folder with both a J.A.C.K. FGD and radiant's
own (the one with a wadlist) gets only radiant's, they define the same classes
==================
*/
std::vector<gameConfig_t> GameConfigs::Detect()
{
	std::vector<gameConfig_t> found;
	std::error_code ec;

	gameConfig_t engine = EngineProfile(".");

	for (const fs::directory_entry &dir : fs::directory_iterator(fs::u8path(EditorDir()), ec))
	{
		if (!dir.is_directory(ec))
			continue;

		std::vector<std::string> all, radiant;
		for (const fs::directory_entry &f : fs::directory_iterator(dir.path(), ec))
		{
			std::string ext = f.path().extension().u8string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
			if (!f.is_regular_file(ec) || ext != ".fgd")
				continue;

			std::string rel = MakeRelative(f.path().u8string());
			all.push_back(rel);

			std::ifstream in(f.path(), std::ios::binary);
			std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			if (text.find("(wadlist)") != std::string::npos)
				radiant.push_back(rel);
		}
		if (all.empty())
			continue;

		gameConfig_t cfg;
		cfg.name = dir.path().filename().u8string();
		cfg.parent = engine.name;
		cfg.fgds = radiant.empty() ? all : radiant;
		std::sort(cfg.fgds.begin(), cfg.fgds.end());
		cfg.gameDir = MakeRelative(dir.path().u8string());
		cfg.compileProfiles = DefaultCompileProfiles(engine.tools);
		found.push_back(cfg);
	}

	std::sort(found.begin(), found.end(), [](const gameConfig_t &a, const gameConfig_t &b) { return a.name < b.name; });
	if (!engine.name.empty())
		found.insert(found.begin(), engine);
	return found;
}

/*
==============================================================================

	RADIANT.INI

	[RadiantGames][Settings]	Active=<name>
	[RadiantGames][Game0]		Name=, Kind=engine|game|mod, Parent=, Fgd= (once per file, in order),
								Tool=<name>|<path> (once per tool), ExecutableDir=, Executable=, LaunchParams=,
								GameDir=, ModDir=, MapsDir=, DefaultPointClass=, DefaultSolidClass=,
								Compile=<name>|<working folder> (once per compilation profile), then
								its tasks: Task=<type>|<enabled>|<flag>|<a>|<b>

	an entry without Kind is from before profiles: a game with the engine's
	folder in it, which moves to an engine profile of its own
==============================================================================
*/

#define SETTINGS_ENTRY	((void *)(intptr_t)-1)

static const char *s_kindKeys[CONFIG_KIND_COUNT] = { "engine", "game", "mod" };
static const char *s_taskKeys[TASK_TYPE_COUNT] = { "export", "copy", "rename", "delete", "run", "launch" };

// a value split at its first count-1 '|'s, the last part keeps the rest
static std::vector<std::string> SplitFields(const std::string &v, size_t count)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (out.size() + 1 < count)
	{
		size_t bar = v.find('|', start);
		if (bar == std::string::npos)
			break;
		out.push_back(v.substr(start, bar - start));
		start = bar + 1;
	}
	out.push_back(v.substr(start));
	out.resize(count);
	return out;
}

static void *Games_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
	if (!strcmp(name, "Settings"))
		return SETTINGS_ENTRY;
	if (strncmp(name, "Game", 4))
		return nullptr;

	GameConfigs::configs.emplace_back();
	GameConfigs::configs.back().kind = -1;		// until a Kind line
	return (void *)(intptr_t)(GameConfigs::configs.size());		// index + 1, the vector may grow under a pointer
}

static void Games_ReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *entry, const char *line)
{
	const char *eq = strchr(line, '=');
	if (!eq)
		return;
	std::string key(line, eq);
	std::string v(eq + 1);

	if (entry == SETTINGS_ENTRY)
	{
		if (key == "Active")
			GameConfigs::activeName = v;
		return;
	}

	gameConfig_t &c = GameConfigs::configs[(size_t)(intptr_t)entry - 1];
	if (key == "Name")						c.name = v;
	else if (key == "Kind")
	{
		c.kind = CONFIG_GAME;
		for (int k = 0; k < CONFIG_KIND_COUNT; k++)
			if (v == s_kindKeys[k])
				c.kind = k;
	}
	else if (key == "Parent")				c.parent = v;
	else if (key == "Fgd")					c.fgds.push_back(v);
	else if (key == "ExecutableDir")		c.executableDir = v;
	else if (key == "Executable")			c.executable = v;
	else if (key == "LaunchParams")			c.launchParams = v;
	else if (key == "Tool")
	{
		std::vector<std::string> f = SplitFields(v, 2);
		if (!f[0].empty())
			c.tools.push_back({ f[0], f[1] });
	}
	else if (key == "Compile")
	{
		std::vector<std::string> f = SplitFields(v, 2);
		c.compileProfiles.emplace_back();
		c.compileProfiles.back().name = f[0];
		c.compileProfiles.back().workDir = f[1];
	}
	else if (key == "Task" && !c.compileProfiles.empty())
	{
		std::vector<std::string> f = SplitFields(v, 5);
		compileTask_t t;
		t.type = -1;
		for (int k = 0; k < TASK_TYPE_COUNT; k++)
			if (f[0] == s_taskKeys[k])
				t.type = k;
		if (t.type < 0)
			return;
		t.enabled = f[1] != "0";
		t.flag = f[2] != "0";
		t.a = f[3];
		t.b = f[4];
		c.compileProfiles.back().tasks.push_back(t);
	}
	else if (key == "GameDir")				c.gameDir = v;
	else if (key == "ModDir")				c.modDir = v;
	else if (key == "MapsDir")				c.mapsDir = v;
	else if (key == "DefaultPointClass")	c.defaultPointClass = v;
	else if (key == "DefaultSolidClass")	c.defaultSolidClass = v;
}

/*
==================
Games_ApplyAll

a configuration without a name can't be picked. one from before profiles
becomes a game on an engine profile for its executable folder
==================
*/
static void Games_ApplyAll(ImGuiContext *, ImGuiSettingsHandler *)
{
	auto &list = GameConfigs::configs;
	list.erase(std::remove_if(list.begin(), list.end(), [](const gameConfig_t &g) { return g.name.empty(); }), list.end());

	std::vector<gameConfig_t> engines;
	for (gameConfig_t &c : list)
	{
		if (c.kind >= 0)
			continue;
		c.kind = CONFIG_GAME;
		if (c.executableDir.empty() || !c.parent.empty())
			continue;

		const gameConfig_t *e = nullptr;
		for (const gameConfig_t &have : engines)
			if (have.executableDir == c.executableDir)
				e = &have;
		if (!e)
		{
			gameConfig_t engine;
			engine.kind = CONFIG_ENGINE;
			engine.name = "Engine";
			for (int n = 2; GameConfigs::Find(list, engine.name) || GameConfigs::Find(engines, engine.name); n++)
				engine.name = "Engine " + std::to_string(n);
			engine.executableDir = c.executableDir;
			gameConfig_t found = EngineProfile(c.executableDir);
			engine.executable = found.executable;
			engine.launchParams = DEFAULT_LAUNCH_PARAMS;
			engine.tools = found.tools;
			engines.push_back(engine);
			e = &engines.back();
		}
		c.parent = e->name;
		c.executableDir.clear();
		if (c.compileProfiles.empty())
			c.compileProfiles = GameConfigs::DefaultCompileProfiles(e->tools);
	}

	if (!engines.empty())
	{
		Log::Info("Game configurations: the engine folder is now an engine profile (%s), Tools > Options", engines.front().name.c_str());
		list.insert(list.begin(), engines.begin(), engines.end());
		ImGui::MarkIniSettingsDirty();
	}
	s_activeValid = false;
}

static void Games_WriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
	buf->appendf("[%s][Settings]\n", handler->TypeName);
	buf->appendf("Active=%s\n\n", GameConfigs::activeName.c_str());

	for (size_t i = 0; i < GameConfigs::configs.size(); i++)
	{
		const gameConfig_t &c = GameConfigs::configs[i];
		buf->appendf("[%s][Game%zu]\n", handler->TypeName, i);
		buf->appendf("Name=%s\n", c.name.c_str());
		buf->appendf("Kind=%s\n", s_kindKeys[c.kind >= 0 && c.kind < CONFIG_KIND_COUNT ? c.kind : CONFIG_GAME]);
		buf->appendf("Parent=%s\n", c.parent.c_str());
		for (const std::string &f : c.fgds)
			buf->appendf("Fgd=%s\n", f.c_str());

		auto put = [buf](const char *key, const std::string &v) { if (!v.empty()) buf->appendf("%s=%s\n", key, v.c_str()); };
		for (const compileTool_t &t : c.tools)
			buf->appendf("Tool=%s|%s\n", t.name.c_str(), t.path.c_str());
		put("ExecutableDir", c.executableDir);
		put("Executable", c.executable);
		put("LaunchParams", c.launchParams);
		put("GameDir", c.gameDir);
		put("ModDir", c.modDir);
		put("MapsDir", c.mapsDir);
		put("DefaultPointClass", c.defaultPointClass);
		put("DefaultSolidClass", c.defaultSolidClass);
		for (const compileProfile_t &p : c.compileProfiles)
		{
			buf->appendf("Compile=%s|%s\n", p.name.c_str(), p.workDir.c_str());
			for (const compileTask_t &t : p.tasks)
				buf->appendf("Task=%s|%d|%d|%s|%s\n", s_taskKeys[t.type], t.enabled ? 1 : 0, t.flag ? 1 : 0, t.a.c_str(), t.b.c_str());
		}
		buf->append("\n");
	}
}

void GameConfigs::RegisterSettings()
{
	ImGuiSettingsHandler handler;
	handler.TypeName = "RadiantGames";
	handler.TypeHash = ImHashStr("RadiantGames");
	handler.ReadOpenFn = Games_ReadOpen;
	handler.ReadLineFn = Games_ReadLine;
	handler.ApplyAllFn = Games_ApplyAll;
	handler.WriteAllFn = Games_WriteAll;
	ImGui::AddSettingsHandler(&handler);
}
