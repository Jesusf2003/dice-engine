//==============================
//	gameConfig.cpp
//==============================

#include "core/common/gameConfig.h"

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

const gameConfig_t *GameConfigs::Active()
{
	for (const gameConfig_t &c : configs)
		if (c.name == activeName)
			return &c;
	return nullptr;
}

void GameConfigs::Apply(const std::vector<gameConfig_t> &list, const std::string &active)
{
	configs = list;
	activeName = active;
	ImGui::MarkIniSettingsDirty();
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
GameConfigs::Detect

every folder next to the editor with an FGD in it is a game. a folder with
both a J.A.C.K. FGD and radiant's own (the one with a wadlist) gets only
radiant's, they define the same classes
==================
*/
std::vector<gameConfig_t> GameConfigs::Detect()
{
	std::vector<gameConfig_t> found;
	std::error_code ec;

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
		cfg.fgds = radiant.empty() ? all : radiant;
		std::sort(cfg.fgds.begin(), cfg.fgds.end());
		cfg.executableDir = ".";
		cfg.gameDir = MakeRelative(dir.path().u8string());
		found.push_back(cfg);
	}

	std::sort(found.begin(), found.end(), [](const gameConfig_t &a, const gameConfig_t &b) { return a.name < b.name; });
	return found;
}

/*
==============================================================================

	RADIANT.INI

	[RadiantGames][Settings]	Active=<name>
	[RadiantGames][Game0]		Name=, Fgd= (once per file, in order), ExecutableDir=,
								GameDir=, ModDir=, MapsDir=, DefaultPointClass=, DefaultSolidClass=
==============================================================================
*/

#define SETTINGS_ENTRY	((void *)(intptr_t)-1)

static void *Games_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
	if (!strcmp(name, "Settings"))
		return SETTINGS_ENTRY;
	if (strncmp(name, "Game", 4))
		return nullptr;

	GameConfigs::configs.emplace_back();
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
	else if (key == "Fgd")					c.fgds.push_back(v);
	else if (key == "ExecutableDir")		c.executableDir = v;
	else if (key == "GameDir")				c.gameDir = v;
	else if (key == "ModDir")				c.modDir = v;
	else if (key == "MapsDir")				c.mapsDir = v;
	else if (key == "DefaultPointClass")	c.defaultPointClass = v;
	else if (key == "DefaultSolidClass")	c.defaultSolidClass = v;
}

static void Games_ApplyAll(ImGuiContext *, ImGuiSettingsHandler *)
{
	// a configuration without a name can't be picked
	auto &c = GameConfigs::configs;
	c.erase(std::remove_if(c.begin(), c.end(), [](const gameConfig_t &g) { return g.name.empty(); }), c.end());
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
		for (const std::string &f : c.fgds)
			buf->appendf("Fgd=%s\n", f.c_str());
		buf->appendf("ExecutableDir=%s\n", c.executableDir.c_str());
		buf->appendf("GameDir=%s\n", c.gameDir.c_str());
		buf->appendf("ModDir=%s\n", c.modDir.c_str());
		buf->appendf("MapsDir=%s\n", c.mapsDir.c_str());
		buf->appendf("DefaultPointClass=%s\n", c.defaultPointClass.c_str());
		buf->appendf("DefaultSolidClass=%s\n", c.defaultSolidClass.c_str());
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
