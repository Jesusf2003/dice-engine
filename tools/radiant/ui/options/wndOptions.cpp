//==============================
//	wndOptions.cpp
//==============================

#include "ui/options/wndOptions.h"
#include "ui/wndMain.h"
#include "core/common/gameConfig.h"
#include "core/common/log.h"
#include "core/utils/fileDialog.h"
#include "map/entityDefs.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

static bool		s_open;
static bool		s_wantFocus;
static std::vector<gameConfig_t>	s_edit;		// what the window shows, GameConfigs once applied
static int		s_current = -1;					// the profile shown
static std::string	s_active;					// the game or mod that becomes the active one once applied
static int		s_selectedFgd = -1;

enum { NAME_NONE, NAME_ADD, NAME_RENAME, NAME_COPY };
static int			s_nameMode;
static std::string	s_nameBuf;
static int			s_newKind;
static std::string	s_newParent;

static const ImVec4 s_missingColor(1.0f, 0.45f, 0.45f, 1.0f);

#ifdef _WIN32
#define PROGRAM_PATTERN	"*.exe"
#else
#define PROGRAM_PATTERN	"*"
#endif

static bool Valid(int i)
{
	return i >= 0 && i < (int)s_edit.size();
}

void WndOptions::Open()
{
	if (!s_open)
	{
		s_edit = GameConfigs::configs;
		s_active = GameConfigs::activeName;
		s_current = -1;
		for (size_t i = 0; i < s_edit.size(); i++)
			if (s_edit[i].name == s_active)
				s_current = (int)i;
		if (s_current < 0 && !s_edit.empty())
			s_current = 0;
		s_selectedFgd = -1;
	}
	s_open = true;
	s_wantFocus = true;
}

// what the definitions and wads are loaded from
static bool SameGame(const gameConfig_t &a, const gameConfig_t &b)
{
	return a.name == b.name && a.fgds == b.fgds && a.gameDir == b.gameDir && a.modDir == b.modDir &&
		a.mapsDir == b.mapsDir && a.defaultPointClass == b.defaultPointClass && a.defaultSolidClass == b.defaultSolidClass;
}

static bool NameTaken(const std::string &name, int except)
{
	for (int i = 0; i < (int)s_edit.size(); i++)
		if (i != except && s_edit[i].name == name)
			return true;
	return false;
}

/*
==================
Apply

the game or mod picked last becomes the active one (the first one there is when
it's gone); its definitions and wads reload when it's a different one or what it
loads changed, its own or inherited
==================
*/
static void Apply()
{
	const gameConfig_t *before = GameConfigs::Active();
	gameConfig_t old = before ? *before : gameConfig_t();
	bool hadActive = before != nullptr;

	const gameConfig_t *pick = GameConfigs::Find(s_edit, s_active);
	if (!pick || !GameConfigs::CanBeActive(pick->kind))
	{
		pick = nullptr;
		for (const gameConfig_t &c : s_edit)
			if (!pick && GameConfigs::CanBeActive(c.kind))
				pick = &c;
	}
	s_active = pick ? pick->name : std::string();
	// the compilation profiles are Run > Compile Map's, which changes them in place meanwhile
	for (gameConfig_t &c : s_edit)
		if (const gameConfig_t *live = GameConfigs::Find(GameConfigs::configs, c.name))
			c.compileProfiles = live->compileProfiles;
	GameConfigs::Apply(s_edit, s_active);

	const gameConfig_t *now = GameConfigs::Active();
	if (!now)
		return;
	if (!hadActive || !SameGame(old, *now))
		WndMain_LoadGame();
}

/*
==================
DisplayOrder

every profile under the one it builds on: engines, their games, the games' mods.
one whose parent is missing goes at the top level
==================
*/
struct displayItem_t { int index, depth; };

static void AddChildren(std::vector<displayItem_t> &out, int parent, int depth)
{
	for (int i = 0; i < (int)s_edit.size(); i++)
	{
		const gameConfig_t *p = GameConfigs::Parent(s_edit, s_edit[i]);
		bool child = parent < 0 ? p == nullptr : p == &s_edit[parent];
		if (!child)
			continue;
		out.push_back({ i, depth });
		AddChildren(out, i, depth + 1);
	}
}

static std::vector<displayItem_t> DisplayOrder()
{
	std::vector<displayItem_t> out;
	AddChildren(out, -1, 0);
	return out;
}

static std::string Label(const gameConfig_t &c)
{
	return std::string(GameConfigs::KindName(c.kind)) + ": " + c.name;
}

// what a profile gets from the ones it builds on
static gameConfig_t Inherited(const gameConfig_t &cfg)
{
	const gameConfig_t *p = GameConfigs::Parent(s_edit, cfg);
	return p ? GameConfigs::Inherited(s_edit, *p) : gameConfig_t();
}

/*
==================
PathRow

label, the path as typed (the inherited one as the hint), Browse..., and whether
it's there. a folder, or a program when pattern is set
==================
*/
static void PathRow(const char *label, std::string &path, const std::string &inherited, const char *hint,
	const char *help, bool optional, const char *pattern = nullptr)
{
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::SetItemTooltip("%s", help);

	std::string hintText = inherited.empty() ? std::string(hint) : "inherited: " + inherited;
	float browseW = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(-browseW - ImGui::GetStyle().ItemSpacing.x);
	ImGui::InputTextWithHint("##path", hintText.c_str(), &path);
	ImGui::SameLine();
	if (ImGui::Button("Browse..."))
	{
		std::string picked;
		const std::string &cur = path.empty() ? inherited : path;
		std::string start = cur.empty() ? GameConfigs::EditorDir() : GameConfigs::Resolve(cur);
		if (pattern)
		{
			if (!cur.empty())
				start = fs::u8path(start).parent_path().u8string();
			if (FileDialog::OpenFile(picked, label, "Programs", pattern, start))
				path = GameConfigs::MakeRelative(picked);
		}
		else if (FileDialog::OpenFolder(picked, label, start))
			path = GameConfigs::MakeRelative(picked);
	}

	const std::string &shown = path.empty() ? inherited : path;
	if (!shown.empty())
	{
		std::error_code ec;
		std::string full = GameConfigs::Resolve(shown);
		fs::path p = fs::u8path(full);
		bool there = pattern ? fs::is_regular_file(p, ec) : fs::is_directory(p, ec);
		if (!there)
			ImGui::TextColored(s_missingColor, "not found: %s", full.c_str());
		else if (full != path)
			ImGui::TextDisabled("%s", full.c_str());
	}
	else if (!optional)
		ImGui::TextColored(s_missingColor, "required");
	ImGui::PopID();
}

/*
==================
ClassRow

a default entity class: typed, or picked from the definitions loaded for the active game
==================
*/
static void ClassRow(const char *label, std::string &value, const std::string &inherited, EntityClass::Type type, bool isActive)
{
	ImGui::PushID(label);
	ImGui::SetNextItemWidth(260);
	ImGui::InputTextWithHint(label, inherited.empty() ? "" : ("inherited: " + inherited).c_str(), &value);
	if (isActive)
	{
		ImGui::SameLine();
		if (ImGui::BeginCombo("##pick", nullptr, ImGuiComboFlags_NoPreview))
		{
			for (const EntityClass &cls : g_entityDefs.classes)
			{
				if (cls.type == type && g_entityDefs.Find(cls.name) == &cls && ImGui::Selectable(cls.name.c_str(), cls.name == value))
					value = cls.name;
			}
			ImGui::EndCombo();
		}
	}
	ImGui::PopID();
}

static void FgdList(gameConfig_t &cfg, const gameConfig_t &inherited)
{
	ImGui::SeparatorText("Game Data files");
	ImGui::SetItemTooltip("The FGDs, loaded in this order: a later one (a mod's) replaces the classes of an earlier one.\n"
		"worldspawn's wadlist in them is the list of wads the texture browser loads.");

	if (!inherited.fgds.empty())
	{
		std::string list;
		for (const std::string &f : inherited.fgds)
			list += (list.empty() ? "" : ", ") + fs::u8path(f).filename().u8string();
		ImGui::TextDisabled("after %s's: %s", inherited.name.c_str(), list.c_str());
	}

	float buttonsW = 90;
	ImGui::BeginChild("##fgds", ImVec2(-buttonsW, ImGui::GetTextLineHeightWithSpacing() * 5.5f), ImGuiChildFlags_Borders);
	for (int i = 0; i < (int)cfg.fgds.size(); i++)
	{
		std::error_code ec;
		bool missing = !fs::is_regular_file(fs::u8path(GameConfigs::Resolve(cfg.fgds[i])), ec);
		if (missing)
			ImGui::PushStyleColor(ImGuiCol_Text, s_missingColor);
		ImGui::PushID(i);
		if (ImGui::Selectable(cfg.fgds[i].c_str(), s_selectedFgd == i))
			s_selectedFgd = i;
		ImGui::PopID();
		if (missing)
		{
			ImGui::PopStyleColor();
			ImGui::SetItemTooltip("not found: %s", GameConfigs::Resolve(cfg.fgds[i]).c_str());
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();
	ImVec2 bs(buttonsW - ImGui::GetStyle().ItemSpacing.x, 0);
	if (ImGui::Button("Add...", bs))
	{
		std::vector<std::string> dirs = GameConfigs::SearchPaths(GameConfigs::Inherited(s_edit, cfg));
		std::string path;
		if (FileDialog::OpenFile(path, "Add Game Data File", "Game data files", "*.fgd", dirs.empty() ? GameConfigs::EditorDir() : dirs.front()))
		{
			cfg.fgds.push_back(GameConfigs::MakeRelative(path));
			s_selectedFgd = (int)cfg.fgds.size() - 1;
		}
	}
	bool sel = s_selectedFgd >= 0 && s_selectedFgd < (int)cfg.fgds.size();
	ImGui::BeginDisabled(!sel);
	if (ImGui::Button("Remove", bs) && sel)
	{
		cfg.fgds.erase(cfg.fgds.begin() + s_selectedFgd);
		s_selectedFgd = -1;
	}
	if (ImGui::Button("Up", bs) && sel && s_selectedFgd > 0)
	{
		std::swap(cfg.fgds[s_selectedFgd], cfg.fgds[s_selectedFgd - 1]);
		s_selectedFgd--;
	}
	if (ImGui::Button("Down", bs) && sel && s_selectedFgd + 1 < (int)cfg.fgds.size())
	{
		std::swap(cfg.fgds[s_selectedFgd], cfg.fgds[s_selectedFgd + 1]);
		s_selectedFgd++;
	}
	ImGui::EndDisabled();
	ImGui::EndGroup();
}

/*
==================
ParentCombo

the profiles of the kind this one can build on, or none
==================
*/
static bool ParentCombo(const char *label, std::string &parent, int kind, const std::string &self)
{
	int want = GameConfigs::ParentKind(kind);
	if (want < 0)
		return false;

	bool changed = false;
	const char *none = kind == CONFIG_MOD ? "(none: not a mod of anything)" : "(none)";
	ImGui::SetNextItemWidth(260);
	if (ImGui::BeginCombo(label, parent.empty() ? none : parent.c_str()))
	{
		if (ImGui::Selectable(none, parent.empty()))
		{
			parent.clear();
			changed = true;
		}
		for (const gameConfig_t &c : s_edit)
		{
			if (c.kind != want || c.name == self)
				continue;
			if (ImGui::Selectable(c.name.c_str(), c.name == parent))
			{
				parent = c.name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	if (!parent.empty())
	{
		const gameConfig_t *p = GameConfigs::Find(s_edit, parent);
		if (!p)
			ImGui::TextColored(s_missingColor, "there's no %s \"%s\"", GameConfigs::KindName(want), parent.c_str());
		else if (p->kind != want)
			ImGui::TextColored(s_missingColor, "\"%s\" is a %s, not a %s", parent.c_str(), GameConfigs::KindName(p->kind), GameConfigs::KindName(want));
	}
	return changed;
}

// what a new profile builds on: the one shown when it fits, else the first that does
static std::string DefaultParent(int kind)
{
	int want = GameConfigs::ParentKind(kind);
	if (want < 0)
		return std::string();
	if (Valid(s_current))
	{
		const gameConfig_t &c = s_edit[s_current];
		if (c.kind == want)
			return c.name;
		if (c.kind == kind && !c.parent.empty())
			return c.parent;
	}
	for (const gameConfig_t &c : s_edit)
		if (c.kind == want)
			return c.name;
	return std::string();
}

static void OpenNamePopup(int mode)
{
	s_nameMode = mode;
	if (mode == NAME_ADD)
	{
		// under the one shown: a game on an engine, a mod of a game
		s_newKind = !Valid(s_current) ? CONFIG_ENGINE : std::min(s_edit[s_current].kind + 1, (int)CONFIG_MOD);
		s_newParent = DefaultParent(s_newKind);
		s_nameBuf = s_newKind == CONFIG_ENGINE ? "New engine" : s_newKind == CONFIG_GAME ? "New game" : "New mod";
	}
	else if (mode == NAME_COPY)
		s_nameBuf = s_edit[s_current].name + " (copy)";
	else
		s_nameBuf = s_edit[s_current].name;
	ImGui::OpenPopup("##name");
}

static void RenameProfile(int index, const std::string &name)
{
	std::string old = s_edit[index].name;
	for (gameConfig_t &c : s_edit)
		if (c.parent == old)
			c.parent = name;
	if (s_active == old)
		s_active = name;
	s_edit[index].name = name;
}

static void RemoveProfile(int index)
{
	std::string old = s_edit[index].name;
	int orphans = 0;
	for (gameConfig_t &c : s_edit)
	{
		if (c.parent == old)
		{
			c.parent.clear();
			orphans++;
		}
	}
	if (orphans)
		Log::Warning("Removed %s \"%s\": %d profiles built on it stand alone now", GameConfigs::KindName(s_edit[index].kind), old.c_str(), orphans);
	if (s_active == old)
		s_active.clear();
	s_edit.erase(s_edit.begin() + index);
	s_current = s_edit.empty() ? -1 : std::min(index, (int)s_edit.size() - 1);
}

/*
==================
NamePopup

add / copy / rename; adding also picks the kind and what it builds on
==================
*/
static void NamePopup()
{
	if (!ImGui::BeginPopup("##name"))
		return;

	const char *title = s_nameMode == NAME_RENAME ? "Rename the profile" : s_nameMode == NAME_COPY ? "Name of the copy" : "New profile";
	ImGui::TextUnformatted(title);

	if (s_nameMode == NAME_ADD)
	{
		for (int k = 0; k < CONFIG_KIND_COUNT; k++)
		{
			if (k)
				ImGui::SameLine();
			if (ImGui::RadioButton(GameConfigs::KindName(k), s_newKind == k))
			{
				s_newKind = k;
				s_newParent = DefaultParent(k);
			}
		}
		ImGui::SetItemTooltip("Engine: the engine and the programs that compile its maps.\n"
			"Game: a base game on an engine.\nMod: a mod of a game, its folder searched before the game's.");
		ParentCombo(s_newKind == CONFIG_MOD ? "Mod of" : "Engine", s_newParent, s_newKind, std::string());
	}

	if (ImGui::IsWindowAppearing())
		ImGui::SetKeyboardFocusHere();
	ImGui::SetNextItemWidth(260);
	bool enter = ImGui::InputText("##newname", &s_nameBuf, ImGuiInputTextFlags_EnterReturnsTrue);

	int except = (s_nameMode == NAME_RENAME) ? s_current : -1;
	bool valid = !s_nameBuf.empty() && !NameTaken(s_nameBuf, except);
	if (!s_nameBuf.empty() && !valid)
		ImGui::TextColored(s_missingColor, "there's one with that name already");

	ImGui::BeginDisabled(!valid);
	if (ImGui::Button("OK", ImVec2(80, 0)) || (enter && valid))
	{
		if (s_nameMode == NAME_RENAME)
			RenameProfile(s_current, s_nameBuf);
		else
		{
			gameConfig_t g;
			if (s_nameMode == NAME_COPY)
				g = s_edit[s_current];
			else
			{
				g.kind = s_newKind;
				g.parent = s_newParent;
				if (g.kind == CONFIG_ENGINE)
				{
					g.executableDir = ".";
					g.launchParams = DEFAULT_LAUNCH_PARAMS;
				}
				else if (g.kind == CONFIG_GAME)
					g.compileProfiles = GameConfigs::DefaultCompileProfiles(Inherited(g).tools);
			}
			g.name = s_nameBuf;
			s_edit.push_back(g);
			s_current = (int)s_edit.size() - 1;
			if (GameConfigs::CanBeActive(g.kind))
				s_active = g.name;
			s_selectedFgd = -1;
		}
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(80, 0)))
		ImGui::CloseCurrentPopup();
	ImGui::EndPopup();
}

/*
==================
ConfigurationBar

hammer's configuration combo, every profile under the one it builds on, with
what its Edit... dialog did as buttons
==================
*/
static void ConfigurationBar()
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Profile:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(300);

	std::string preview = Valid(s_current) ? Label(s_edit[s_current]) : "(none)";
	if (ImGui::BeginCombo("##config", preview.c_str(), ImGuiComboFlags_HeightLarge))
	{
		for (const displayItem_t &d : DisplayOrder())
		{
			const gameConfig_t &c = s_edit[d.index];
			std::string label = std::string(d.depth * 4, ' ') + Label(c);
			if (c.name == s_active)
				label += "  (active)";
			ImGui::PushID(d.index);
			if (ImGui::Selectable(label.c_str(), d.index == s_current))
			{
				s_current = d.index;
				s_selectedFgd = -1;
				if (GameConfigs::CanBeActive(c.kind))
					s_active = c.name;
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	ImGui::SetItemTooltip("The engine, game or mod to edit. OK or Apply makes the game or mod picked the active one.");

	bool has = Valid(s_current);
	ImGui::SameLine();
	if (ImGui::Button("Add"))
		OpenNamePopup(NAME_ADD);
	ImGui::SetItemTooltip("A new engine, game or mod: by default a game on the engine shown, a mod of the game shown");
	ImGui::BeginDisabled(!has);
	ImGui::SameLine();
	if (ImGui::Button("Copy"))
		OpenNamePopup(NAME_COPY);
	ImGui::SetItemTooltip("A new profile of the same kind with the same files and folders");
	ImGui::SameLine();
	if (ImGui::Button("Rename"))
		OpenNamePopup(NAME_RENAME);
	ImGui::SameLine();
	if (ImGui::Button("Remove") && has)
		RemoveProfile(s_current);
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Detect"))
	{
		int added = 0;
		for (const gameConfig_t &g : GameConfigs::Detect())
		{
			if (NameTaken(g.name, -1))
				continue;
			s_edit.push_back(g);
			added++;
		}
		Log::Info("Detect: %d new profiles next to the editor", added);
		if (s_current < 0 && !s_edit.empty())
			s_current = 0;
	}
	ImGui::SetItemTooltip("Adds an engine profile for the engine next to the editor, and a game on it for every folder next to the editor with an FGD in it");

	NamePopup();
}

/*
==================
ToolsTable

the compilation tools, trenchbroom's game "compilationTools" with their paths:
a name (${name} in the compilation profiles) and a program. a game's replace
its engine's of the same name, which are listed under them
==================
*/
static void ToolsTable(gameConfig_t &cfg, const gameConfig_t &inherited)
{
	ImGui::SeparatorText("Compilation Tools");
	ImGui::SetItemTooltip("The programs Run > Compile Map runs: ${name} in a profile's tasks is the path of the tool of that name.");

	int remove = -1;
	if (!cfg.tools.empty() && ImGui::BeginTable("##tools", 4, ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 110);
		ImGui::TableSetupColumn("path", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("browse", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("remove", ImGuiTableColumnFlags_WidthFixed);
		for (int i = 0; i < (int)cfg.tools.size(); i++)
		{
			compileTool_t &t = cfg.tools[i];
			ImGui::PushID(i);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##name", "qbsp", &t.name);
			ImGui::SetItemTooltip("${%s}", t.name.c_str());
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##path", "the program", &t.path);
			std::error_code ec;
			if (!t.path.empty() && !fs::is_regular_file(fs::u8path(GameConfigs::Resolve(t.path)), ec))
				ImGui::TextColored(s_missingColor, "not found: %s", GameConfigs::Resolve(t.path).c_str());
			ImGui::TableNextColumn();
			if (ImGui::Button("Browse..."))
			{
				std::string picked, start = t.path.empty() ? GameConfigs::EditorDir() : fs::u8path(GameConfigs::Resolve(t.path)).parent_path().u8string();
				if (FileDialog::OpenFile(picked, "Compilation tool", "Programs", PROGRAM_PATTERN, start))
				{
					t.path = GameConfigs::MakeRelative(picked);
					if (t.name.empty())
						t.name = fs::u8path(picked).stem().u8string();
				}
			}
			ImGui::TableNextColumn();
			if (ImGui::Button("X"))
				remove = i;
			ImGui::SetItemTooltip("Remove the tool");
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	if (remove >= 0)
		cfg.tools.erase(cfg.tools.begin() + remove);

	for (const compileTool_t &t : inherited.tools)
	{
		if (GameConfigs::FindTool(cfg, t.name))
			continue;
		ImGui::PushID(t.name.c_str());
		ImGui::TextDisabled("${%s}  %s  (from %s)", t.name.c_str(), GameConfigs::Resolve(t.path).c_str(), inherited.name.c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Override"))
			cfg.tools.push_back(t);
		ImGui::PopID();
	}

	if (ImGui::Button("Add Tool"))
		cfg.tools.push_back({ cfg.tools.empty() && inherited.tools.empty() ? "qbsp" : "", "" });
	ImGui::SameLine();
	if (ImGui::Button("Find in Folder..."))
	{
		std::string dir;
		if (FileDialog::OpenFolder(dir, "Folder with the compilation tools", GameConfigs::EditorDir()))
		{
			std::vector<compileTool_t> found = GameConfigs::DetectTools(GameConfigs::MakeRelative(dir));
			for (const compileTool_t &t : found)
			{
				auto same = std::find_if(cfg.tools.begin(), cfg.tools.end(), [&](const compileTool_t &o) { return o.name == t.name; });
				if (same != cfg.tools.end())
					same->path = t.path;
				else
					cfg.tools.push_back(t);
			}
			Log::Info("Found %zu compilation tools (qbsp, vis, light, vmap) in %s", found.size(), dir.c_str());
		}
	}
	ImGui::SetItemTooltip("Adds qbsp, vis, light and vmap from a folder, ericw-tools' bin for one");
}

static void EngineProfile(gameConfig_t &cfg)
{
	FgdList(cfg, gameConfig_t());

	ImGui::SeparatorText("Engine");
	ImGui::TextDisabled("relative paths are from the editor's folder: %s", GameConfigs::EditorDir().c_str());
	PathRow("Game Executable Directory", cfg.executableDir, std::string(), "the engine's folder, its -basedir",
		"Where the engine is: the folder the games are in. ${ENGINE_DIR_PATH}, and where it runs from.", false);
	PathRow("Game Executable", cfg.executable, std::string(), "fteqw64.exe",
		"The engine program, Run > Launch Engine and the compilation profiles' Launch Engine task.", false, PROGRAM_PATTERN);

	ImGui::TextUnformatted("Launch parameters");
	ImGui::SetItemTooltip("Trenchbroom's engine profile parameters: ${VARIABLE}s are filled in for the open map (Run > Launch Engine lists them)");
	float resetW = ImGui::CalcTextSize("Default").x + ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(-resetW - ImGui::GetStyle().ItemSpacing.x);
	ImGui::InputTextWithHint("##launch", DEFAULT_LAUNCH_PARAMS, &cfg.launchParams);
	ImGui::SameLine();
	if (ImGui::Button("Default"))
		cfg.launchParams = DEFAULT_LAUNCH_PARAMS;

	ToolsTable(cfg, gameConfig_t());
}

static void GameOrModProfile(gameConfig_t &cfg)
{
	bool isMod = cfg.kind == CONFIG_MOD;
	bool isActive = cfg.name == GameConfigs::activeName;

	ImGui::SeparatorText(isMod ? "Mod of" : "Engine");
	ParentCombo(isMod ? "Game" : "Engine", cfg.parent, cfg.kind, cfg.name);
	ImGui::SetItemTooltip(isMod ? "The game this is a mod of: its folder, FGDs and engine come from it."
		: "The engine the game runs on: its folder, launch parameters and compilation tools come from it.");

	gameConfig_t up = Inherited(cfg);
	FgdList(cfg, up);

	ImGui::SeparatorText("Folders");
	ImGui::TextDisabled("relative paths are from the editor's folder: %s", GameConfigs::EditorDir().c_str());
	if (isMod)
	{
		PathRow("Mod Directory", cfg.modDir, std::string(), "the mod's folder, searched first",
			"The mod's folder, searched before its game's: wads in the wadlist and gfx/palette.lmp are looked for here first.", false);
		if (!cfg.gameDir.empty())
			PathRow("Game Directory", cfg.gameDir, up.gameDir, "", "Replaces the game's folder. Leave empty to use the game's.", true);
	}
	else
	{
		PathRow("Game Directory", cfg.gameDir, std::string(), "the base game: ninjawars",
			"The base game's folder, like quake's id1. Wads in the wadlist and gfx/palette.lmp are looked for here.", false);
		if (!cfg.modDir.empty())
			PathRow("Mod Directory", cfg.modDir, std::string(), "", "From before profiles: make it a mod profile of this game instead.", true);
	}
	PathRow("Maps Directory", cfg.mapsDir, std::string(), isMod ? "optional: <mod>/maps" : "optional: <game>/maps",
		"Where File > Open starts.", true);

	if (!isMod)
	{
		ToolsTable(cfg, up);
		ImGui::TextDisabled("compilation profiles: %zu (Run > Compile Map)", cfg.compileProfiles.size());
	}

	ImGui::SeparatorText("New entities");
	ClassRow("Default PointEntity class", cfg.defaultPointClass, up.defaultPointClass, EntityClass::POINT, isActive);
	ClassRow("Default SolidEntity class", cfg.defaultSolidClass, up.defaultSolidClass, EntityClass::SOLID, isActive);
	if (!isActive)
		ImGui::TextDisabled("the class lists come from the active profile's definitions");

	// what the editor ends up with
	gameConfig_t all = GameConfigs::Inherited(s_edit, cfg);
	ImGui::SeparatorText("In use");
	std::vector<std::string> dirs = GameConfigs::SearchPaths(all);
	std::string order;
	for (const std::string &d : dirs)
		order += (order.empty() ? "" : "  >  ") + fs::u8path(d).filename().u8string();
	ImGui::TextDisabled("search path: %s", order.empty() ? "(none)" : order.c_str());
	ImGui::TextDisabled("game data files: %zu", all.fgds.size());
	ImGui::TextDisabled("engine: %s", all.executable.empty() ? "(none)" : GameConfigs::Resolve(all.executable).c_str());
	std::string tools;
	for (const compileTool_t &t : all.tools)
		tools += (tools.empty() ? "" : ", ") + t.name;
	ImGui::TextDisabled("compilation tools: %s", tools.empty() ? "(none)" : tools.c_str());
}

static void GameConfigurationsTab()
{
	ConfigurationBar();
	ImGui::Separator();

	if (!Valid(s_current))
	{
		ImGui::TextDisabled("No profiles. Add an engine, a game on it and its mods, or Detect the ones next to the editor.");
		return;
	}

	gameConfig_t &cfg = s_edit[s_current];
	if (cfg.kind == CONFIG_ENGINE)
		EngineProfile(cfg);
	else
		GameOrModProfile(cfg);
}

/*
==================
WndOptions::Draw
==================
*/
void WndOptions::Draw()
{
	if (!s_open)
		return;

	ImGui::SetNextWindowSize(ImVec2(760, 680), ImGuiCond_FirstUseEver);
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x / 2, vp->WorkPos.y + vp->WorkSize.y / 2),
		ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	if (s_wantFocus)
	{
		ImGui::SetNextWindowFocus();
		s_wantFocus = false;
	}

	bool open = true;
	if (!ImGui::Begin("Options###RadiantOptions", &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		s_open = open;
		return;
	}

	float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
	ImGui::BeginChild("##tabs", ImVec2(0, -footer));
	if (ImGui::BeginTabBar("##optionTabs"))
	{
		if (ImGui::BeginTabItem("Game Configurations"))
		{
			GameConfigurationsTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::EndChild();

	ImGui::Separator();
	float bw = 80;
	ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - bw * 3 - ImGui::GetStyle().ItemSpacing.x * 2);
	if (ImGui::Button("OK", ImVec2(bw, 0)))
	{
		Apply();
		open = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Apply", ImVec2(bw, 0)))
		Apply();
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(bw, 0)) ||
		(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId) &&
		 ImGui::IsKeyPressed(ImGuiKey_Escape)))
		open = false;

	ImGui::End();
	s_open = open;
}
