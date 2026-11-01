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
static int		s_current = -1;					// the configuration shown, and the active one once applied
static int		s_selectedFgd = -1;

enum { NAME_NONE, NAME_ADD, NAME_RENAME, NAME_COPY };
static int			s_nameMode;
static std::string	s_nameBuf;

static const ImVec4 s_missingColor(1.0f, 0.45f, 0.45f, 1.0f);

void WndOptions::Open()
{
	if (!s_open)
	{
		s_edit = GameConfigs::configs;
		s_current = -1;
		for (size_t i = 0; i < s_edit.size(); i++)
			if (s_edit[i].name == GameConfigs::activeName)
				s_current = (int)i;
		if (s_current < 0 && !s_edit.empty())
			s_current = 0;
		s_selectedFgd = -1;
	}
	s_open = true;
	s_wantFocus = true;
}

static bool SameConfig(const gameConfig_t &a, const gameConfig_t &b)
{
	return a.name == b.name && a.fgds == b.fgds && a.executableDir == b.executableDir && a.gameDir == b.gameDir &&
		a.modDir == b.modDir && a.mapsDir == b.mapsDir &&
		a.defaultPointClass == b.defaultPointClass && a.defaultSolidClass == b.defaultSolidClass;
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

the selected configuration becomes the active one; its definitions and wads
reload when it's a different game or its files or folders changed
==================
*/
static void Apply()
{
	const gameConfig_t *before = GameConfigs::Active();
	gameConfig_t old = before ? *before : gameConfig_t();
	bool hadActive = before != nullptr;

	std::string active = (s_current >= 0 && s_current < (int)s_edit.size()) ? s_edit[s_current].name : std::string();
	GameConfigs::Apply(s_edit, active);

	const gameConfig_t *now = GameConfigs::Active();
	if (!now)
		return;
	if (!hadActive || !SameConfig(old, *now))
		WndMain_LoadGame();
}

/*
==================
FolderRow

label, the path as typed, Browse..., and whether it's there
==================
*/
static void FolderRow(const char *label, std::string &path, const char *hint, const char *help, bool optional)
{
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	ImGui::SetItemTooltip("%s", help);

	float browseW = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(-browseW - ImGui::GetStyle().ItemSpacing.x);
	ImGui::InputTextWithHint("##path", hint, &path);
	ImGui::SameLine();
	if (ImGui::Button("Browse..."))
	{
		std::string picked;
		std::string start = path.empty() ? GameConfigs::EditorDir() : GameConfigs::Resolve(path);
		if (FileDialog::OpenFolder(picked, label, start))
			path = GameConfigs::MakeRelative(picked);
	}

	if (!path.empty())
	{
		std::error_code ec;
		std::string full = GameConfigs::Resolve(path);
		if (!fs::is_directory(fs::u8path(full), ec))
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
static void ClassRow(const char *label, std::string &value, EntityClass::Type type, bool isActive)
{
	ImGui::PushID(label);
	ImGui::SetNextItemWidth(260);
	ImGui::InputText(label, &value);
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

static void FgdList(gameConfig_t &cfg)
{
	ImGui::SeparatorText("Game Data files");
	ImGui::SetItemTooltip("The FGDs, loaded in this order: a later one (a mod's) replaces the classes of an earlier one.\n"
		"worldspawn's wadlist in them is the list of wads the texture browser loads.");

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
		std::vector<std::string> dirs = GameConfigs::SearchPaths(cfg);
		std::string path;
		if (FileDialog::OpenFile(path, "Add Game Data File", "Game data files", "*.fgd", dirs.empty() ? GameConfigs::EditorDir() : dirs.back()))
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
ConfigurationBar

hammer's configuration combo, with what its Edit... dialog did as buttons
==================
*/
static void ConfigurationBar()
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Configuration:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(260);

	const char *preview = (s_current >= 0 && s_current < (int)s_edit.size()) ? s_edit[s_current].name.c_str() : "(none)";
	if (ImGui::BeginCombo("##config", preview))
	{
		for (int i = 0; i < (int)s_edit.size(); i++)
		{
			std::string label = s_edit[i].name;
			if (s_edit[i].name == GameConfigs::activeName)
				label += "  (active)";
			ImGui::PushID(i);
			if (ImGui::Selectable(label.c_str(), i == s_current))
			{
				s_current = i;
				s_selectedFgd = -1;
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	ImGui::SetItemTooltip("The game or mod to map for. OK or Apply makes it the active one.");

	bool has = s_current >= 0 && s_current < (int)s_edit.size();
	ImGui::SameLine();
	if (ImGui::Button("Add"))
	{
		s_nameMode = NAME_ADD;
		s_nameBuf = "New game";
		ImGui::OpenPopup("##name");
	}
	ImGui::BeginDisabled(!has);
	ImGui::SameLine();
	if (ImGui::Button("Copy"))
	{
		s_nameMode = NAME_COPY;
		s_nameBuf = s_edit[s_current].name + " (mod)";
		ImGui::OpenPopup("##name");
	}
	ImGui::SetItemTooltip("A new configuration with the same files and folders, the start of a mod's");
	ImGui::SameLine();
	if (ImGui::Button("Rename"))
	{
		s_nameMode = NAME_RENAME;
		s_nameBuf = s_edit[s_current].name;
		ImGui::OpenPopup("##name");
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove") && has)
	{
		s_edit.erase(s_edit.begin() + s_current);
		s_current = s_edit.empty() ? -1 : std::min(s_current, (int)s_edit.size() - 1);
	}
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
		Log::Info("Detect: %d new game configurations next to the editor", added);
		if (s_current < 0 && !s_edit.empty())
			s_current = 0;
	}
	ImGui::SetItemTooltip("Adds a configuration for every folder next to the editor with an FGD in it");

	// the name popup for add / copy / rename
	if (ImGui::BeginPopup("##name"))
	{
		ImGui::TextUnformatted(s_nameMode == NAME_RENAME ? "Rename the configuration" : "Name of the new configuration");
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
				s_edit[s_current].name = s_nameBuf;
			else
			{
				gameConfig_t g;
				if (s_nameMode == NAME_COPY)
					g = s_edit[s_current];
				else
					g.executableDir = ".";
				g.name = s_nameBuf;
				s_edit.push_back(g);
				s_current = (int)s_edit.size() - 1;
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
}

static void GameConfigurationsTab()
{
	ConfigurationBar();
	ImGui::Separator();

	if (s_current < 0 || s_current >= (int)s_edit.size())
	{
		ImGui::TextDisabled("No game configurations. Add one, or Detect the games next to the editor.");
		return;
	}

	gameConfig_t &cfg = s_edit[s_current];
	bool isActive = cfg.name == GameConfigs::activeName;

	FgdList(cfg);

	ImGui::SeparatorText("Folders");
	ImGui::TextDisabled("relative paths are from the editor's folder: %s", GameConfigs::EditorDir().c_str());
	FolderRow("Game Executable Directory", cfg.executableDir, "the engine's folder",
		"Where the engine executable is, to run the maps (later).", false);
	FolderRow("Game Directory", cfg.gameDir, "the base game: ninjawars",
		"The base game's folder, like quake's id1. Wads in the wadlist and gfx/palette.lmp are looked for here.", false);
	FolderRow("Mod Directory", cfg.modDir, "optional: a mod's folder, searched first",
		"For a mod: its folder, searched before the game's. Leave empty for the base game.", true);
	FolderRow("Maps Directory", cfg.mapsDir, "optional: <mod or game>/maps",
		"Where File > Open starts.", true);

	std::vector<std::string> dirs = GameConfigs::SearchPaths(cfg);
	if (!dirs.empty())
	{
		std::string order;
		for (const std::string &d : dirs)
			order += (order.empty() ? "" : "  >  ") + fs::u8path(d).filename().u8string();
		ImGui::TextDisabled("wads and the palette are looked for in: %s", order.c_str());
	}

	ImGui::SeparatorText("New entities");
	ClassRow("Default PointEntity class", cfg.defaultPointClass, EntityClass::POINT, isActive);
	ClassRow("Default SolidEntity class", cfg.defaultSolidClass, EntityClass::SOLID, isActive);
	if (!isActive)
		ImGui::TextDisabled("the class lists come from the active configuration's definitions");
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

	ImGui::SetNextWindowSize(ImVec2(760, 620), ImGuiCond_FirstUseEver);
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
