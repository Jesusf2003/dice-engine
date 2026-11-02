//==============================
//	wndRun.cpp
//==============================

#include "ui/run/wndRun.h"
#include "ui/options/wndOptions.h"
#include "core/compile/compilation.h"
#include "core/common/gameConfig.h"
#include "core/utils/fileDialog.h"
#include "map/map.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cstring>
#include <filesystem>

#include <algorithm>

static const ImVec4 s_errorColor(1.0f, 0.45f, 0.45f, 1.0f);
static const ImVec4 s_taskColor(0.55f, 0.75f, 1.0f, 1.0f);
static const ImVec4 s_successColor(0.5f, 0.9f, 0.5f, 1.0f);

#ifdef _WIN32
#define PROGRAM_PATTERN	"*.exe"
#else
#define PROGRAM_PATTERN	"*"
#endif

/*
==================
VariablesButton

a button whose popup lists the ${VARIABLE}s with what they are for the open
map, trenchbroom's variable completion as a table to copy from
==================
*/
static void VariablesButton(const compileVars_t &vars, bool compiling)
{
	if (ImGui::Button("Variables..."))
		ImGui::OpenPopup("##variables");
	ImGui::SetItemTooltip("The ${VARIABLE}s the fields can use, and what they are now");

	if (!ImGui::BeginPopup("##variables"))
		return;
	ImGui::TextDisabled("click a name to copy it");
	if (ImGui::BeginTable("##vars", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
	{
		auto row = [](const std::string &name, const std::string &value, const char *help) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			std::string token = "${" + name + "}";
			if (ImGui::Selectable(token.c_str()))
				ImGui::SetClipboardText(token.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(value.empty() ? "(empty)" : value.c_str());
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", help);
		};
		for (const Compilation::variableHelp_t *h = Compilation::variableHelp; h->name; h++)
		{
			auto it = vars.find(h->name);
			if (it == vars.end() && !(compiling && !strcmp(h->name, "WORK_DIR_PATH")))
				continue;
			row(h->name, it != vars.end() ? it->second : std::string("(the profile's)"), h->help);
		}
		for (const auto &v : vars)
		{
			bool known = false;
			for (const Compilation::variableHelp_t *h = Compilation::variableHelp; h->name; h++)
				known |= v.first == h->name;
			if (!known)
				row(v.first, v.second, "compilation tool");
		}
		ImGui::EndTable();
	}
	ImGui::EndPopup();
}

/*
==============================================================================

	COMPILE MAP

==============================================================================
*/

static bool		s_compileOpen;
static bool		s_compileFocus;
static int		s_profile;
static std::vector<compileLine_t>	s_output;
static size_t	s_outputTaken;
static bool		s_scrollOutput;

void WndCompile::Open()
{
	s_compileOpen = true;
	s_compileFocus = true;
}

// a field with its own id, a hint and the help as tooltip; true when changed
static bool Field(const char *label, std::string &value, const char *hint, const char *help, float width = -1)
{
	ImGui::SetNextItemWidth(width);
	bool changed = ImGui::InputTextWithHint(label, hint, &value);
	if (help)
		ImGui::SetItemTooltip("%s", help);
	return changed;
}

/*
==================
ToolField

trenchbroom's tool spec: usually a ${tool}, picked from the game's tools, or a program
==================
*/
static bool ToolField(std::string &value, const gameConfig_t &game)
{
	bool changed = false;
	float pickW = ImGui::GetFrameHeight();
	float browseW = ImGui::CalcTextSize("Browse...").x + ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SetNextItemWidth(-(pickW + browseW + ImGui::GetStyle().ItemSpacing.x * 2));
	changed |= ImGui::InputTextWithHint("##tool", "${qbsp}, or a program", &value);
	ImGui::SetItemTooltip("The program to run: ${name} for one of the compilation tools (Tools > Options), or a path");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##pickTool", nullptr, ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft))
	{
		if (game.tools.empty())
			ImGui::TextDisabled("no compilation tools: add them in Tools > Options");
		for (const compileTool_t &t : game.tools)
		{
			std::string token = "${" + t.name + "}";
			std::string label = token + "  " + t.path;
			if (ImGui::Selectable(label.c_str(), value == token))
			{
				value = token;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Browse..."))
	{
		std::string picked;
		if (FileDialog::OpenFile(picked, "Tool", "Programs", PROGRAM_PATTERN, GameConfigs::EditorDir()))
		{
			value = picked;
			changed = true;
		}
	}
	return changed;
}

/*
==================
TaskEditor

one task, trenchbroom's CompilationTaskEditor: enabled, its fields, moved and
removed with the buttons on its title line. true when changed
==================
*/
enum { TASK_KEEP, TASK_UP, TASK_DOWN, TASK_REMOVE };

static bool TaskEditor(compileTask_t &t, int index, int count, const gameConfig_t &game, const compileVars_t &vars,
	bool current, int &action)
{
	bool changed = false;
	ImGui::PushID(index);

	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 border = current ? s_taskColor : style.Colors[ImGuiCol_Border];
	ImGui::PushStyleColor(ImGuiCol_Border, border);
	ImGui::BeginChild("##task", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

	changed |= ImGui::Checkbox("##enabled", &t.enabled);
	ImGui::SetItemTooltip("Run this task");
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::Text("%d. %s", index + 1, Compilation::TaskName(t.type));

	float bw = ImGui::GetFrameHeight();
	ImGui::SameLine(ImGui::GetContentRegionMax().x - bw * 3 - style.ItemSpacing.x * 2);
	ImGui::BeginDisabled(index == 0);
	if (ImGui::ArrowButton("##up", ImGuiDir_Up))
		action = TASK_UP;
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(index + 1 >= count);
	if (ImGui::ArrowButton("##down", ImGuiDir_Down))
		action = TASK_DOWN;
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("X", ImVec2(bw, bw)))
		action = TASK_REMOVE;
	ImGui::SetItemTooltip("Remove the task");

	ImGui::BeginDisabled(!t.enabled);
	switch (t.type)
	{
	case TASK_EXPORT_MAP:
		changed |= Field("##target", t.a, "${WORK_DIR_PATH}/${MAP_BASE_NAME}-compile.map",
			"Where the map is written, relative to the working folder.\nThe editor can't save yet: it's the map file as it is on disk.");
		break;

	case TASK_COPY_FILES:
	case TASK_RENAME_FILE:
	{
		bool copy = t.type == TASK_COPY_FILES;
		float half = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(copy ? "into" : "to").x - style.ItemSpacing.x * 2) / 2;
		changed |= Field("##source", t.a, copy ? "${MAP_BASE_NAME}.bsp" : "the file",
			copy ? "The files: a file name pattern (* and ?), relative to the working folder" : "The file, relative to the working folder", half);
		ImGui::SameLine();
		ImGui::TextUnformatted(copy ? "into" : "to");
		ImGui::SameLine();
		changed |= Field("##target", t.b, copy ? "${MAPS_DIR_PATH}" : "its new name",
			copy ? "The folder they're copied into" : "Its new path, relative to the working folder", -1);
		break;
	}

	case TASK_DELETE_FILES:
		changed |= Field("##target", t.a, "${MAP_BASE_NAME}.prt",
			"The files: a file name pattern (* and ?) in a folder, relative to the working folder");
		break;

	case TASK_RUN_TOOL:
		changed |= ToolField(t.a, game);
		changed |= Field("##params", t.b, "parameters: \"${MAP_FULL_NAME}\"",
			"Its parameters, quoted the way the command line wants them");
		changed |= ImGui::Checkbox("Treat a non-zero exit code as an error", &t.flag);
		break;

	case TASK_LAUNCH_ENGINE:
	{
		std::string program, parameters, workDir, error;
		if (Compilation::EngineCommand(game, vars, program, parameters, workDir, error))
			ImGui::TextDisabled("%s %s", program.c_str(), parameters.c_str());
		else
			ImGui::TextColored(s_errorColor, "%s", error.c_str());
		changed |= ImGui::Checkbox("Treat a launch failure as an error", &t.flag);
		break;
	}
	}

	// what the fields come to for the open map
	if (t.type != TASK_LAUNCH_ENGINE)
	{
		std::string a, b, error;
		bool ok = Compilation::Interpolate(t.a, vars, a, error);
		if (ok && (t.type == TASK_COPY_FILES || t.type == TASK_RENAME_FILE || t.type == TASK_RUN_TOOL))
			ok = Compilation::Interpolate(t.b, vars, b, error);
		if (!ok)
			ImGui::TextColored(s_errorColor, "%s", error.c_str());
		else if (t.type == TASK_RUN_TOOL && a.empty())
			ImGui::TextColored(s_errorColor, "the tool has no path: set it in Tools > Options");
	}
	ImGui::EndDisabled();

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopID();
	return changed;
}

/*
==================
ProfileList

the game's profiles, trenchbroom's CompilationProfileManager list and its buttons
==================
*/
static bool ProfileList(gameConfig_t &owner, const gameConfig_t &game)
{
	bool changed = false;
	std::vector<compileProfile_t> &list = owner.compileProfiles;

	ImGui::TextDisabled("%s's profiles", owner.name.c_str());
	float buttonsH = ImGui::GetFrameHeightWithSpacing() * 2;
	ImGui::BeginChild("##profiles", ImVec2(0, -buttonsH), ImGuiChildFlags_Borders);
	for (int i = 0; i < (int)list.size(); i++)
	{
		ImGui::PushID(i);
		const char *name = list[i].name.empty() ? "(unnamed)" : list[i].name.c_str();
		if (ImGui::Selectable(name, i == s_profile))
			s_profile = i;
		ImGui::PopID();
	}
	if (list.empty())
		ImGui::TextDisabled("none yet");
	ImGui::EndChild();

	bool sel = s_profile >= 0 && s_profile < (int)list.size();
	if (ImGui::Button("Add"))
	{
		compileProfile_t p;
		p.name = "unnamed";
		list.push_back(p);
		s_profile = (int)list.size() - 1;
		changed = true;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!sel);
	if (ImGui::Button("Copy") && sel)
	{
		compileProfile_t p = list[s_profile];
		p.name += " (copy)";
		list.insert(list.begin() + s_profile + 1, p);
		s_profile++;
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove") && sel)
	{
		list.erase(list.begin() + s_profile);
		s_profile = std::min(s_profile, (int)list.size() - 1);
		changed = true;
	}
	ImGui::EndDisabled();

	ImGui::BeginDisabled(!sel || s_profile == 0);
	if (ImGui::ArrowButton("##pup", ImGuiDir_Up) && sel && s_profile > 0)
	{
		std::swap(list[s_profile], list[s_profile - 1]);
		s_profile--;
		changed = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!sel || s_profile + 1 >= (int)list.size());
	if (ImGui::ArrowButton("##pdown", ImGuiDir_Down) && sel && s_profile + 1 < (int)list.size())
	{
		std::swap(list[s_profile], list[s_profile + 1]);
		s_profile++;
		changed = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Defaults"))
	{
		int added = 0;
		for (const compileProfile_t &p : GameConfigs::DefaultCompileProfiles(game.tools))
		{
			if (std::none_of(list.begin(), list.end(), [&](const compileProfile_t &o) { return o.name == p.name; }))
			{
				list.push_back(p);
				added++;
			}
		}
		if (added)
		{
			s_profile = (int)list.size() - 1;
			changed = true;
		}
	}
	ImGui::SetItemTooltip("Adds the default profiles for the compilation tools the game has: qbsp, vis and light, or vmap.\n%s",
		game.tools.empty() ? "It has none yet, set them up in Tools > Options." : "");
	return changed;
}

static bool ProfileEditor(compileProfile_t &p, const gameConfig_t &game, const compileVars_t &mapVars, bool running)
{
	bool changed = false;
	changed |= Field("Name", p.name, "the profile's name", nullptr, 300);
	changed |= Field("Working folder", p.workDir, "${MAP_DIR_PATH}",
		"Where the tools run; relative paths in the tasks are from here. Created when it isn't there", 300);

	// the tasks see the working folder as the run will
	compileVars_t vars = mapVars;
	{
		std::string dir, error;
		if (Compilation::Interpolate(p.workDir, vars, dir, error))
		{
			vars["WORK_DIR_PATH"] = dir.empty() ? dir : std::filesystem::u8path(dir).lexically_normal().u8string();
			ImGui::TextDisabled("%s", dir.empty() ? "(no map open)" : vars["WORK_DIR_PATH"].c_str());
		}
		else
			ImGui::TextColored(s_errorColor, "%s", error.c_str());
	}

	ImGui::SeparatorText("Tasks");
	ImGui::BeginChild("##tasks", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
	int action = TASK_KEEP, actionIndex = -1;
	for (int i = 0; i < (int)p.tasks.size(); i++)
	{
		int a = TASK_KEEP;
		bool current = running && g_compileRunner.CurrentTask() == i;
		changed |= TaskEditor(p.tasks[i], i, (int)p.tasks.size(), game, vars, current, a);
		if (a != TASK_KEEP)
		{
			action = a;
			actionIndex = i;
		}
	}
	if (p.tasks.empty())
		ImGui::TextDisabled("no tasks: add one below");
	ImGui::EndChild();

	if (actionIndex >= 0)
	{
		if (action == TASK_UP && actionIndex > 0)
			std::swap(p.tasks[actionIndex], p.tasks[actionIndex - 1]);
		else if (action == TASK_DOWN && actionIndex + 1 < (int)p.tasks.size())
			std::swap(p.tasks[actionIndex], p.tasks[actionIndex + 1]);
		else if (action == TASK_REMOVE)
			p.tasks.erase(p.tasks.begin() + actionIndex);
		changed = true;
	}

	if (ImGui::Button("Add Task..."))
		ImGui::OpenPopup("##addTask");
	if (ImGui::BeginPopup("##addTask"))
	{
		for (int type = 0; type < TASK_TYPE_COUNT; type++)
		{
			if (!ImGui::MenuItem(Compilation::TaskName(type)))
				continue;
			compileTask_t t;
			t.type = type;
			if (type == TASK_EXPORT_MAP)
				t.a = "${WORK_DIR_PATH}/${MAP_BASE_NAME}-compile.map";
			else if (type == TASK_COPY_FILES)
			{
				t.a = "${MAP_BASE_NAME}.bsp";
				t.b = "${MAPS_DIR_PATH}";
			}
			else if (type == TASK_RUN_TOOL && !game.tools.empty())
			{
				t.a = "${" + game.tools.front().name + "}";
				t.b = "\"${MAP_FULL_NAME}\"";
			}
			p.tasks.push_back(t);
			changed = true;
		}
		ImGui::EndPopup();
	}
	ImGui::SameLine();
	VariablesButton(vars, true);
	return changed;
}

static void Output()
{
	s_outputTaken = g_compileRunner.TakeLines(s_output, s_outputTaken);

	ImGui::BeginChild("##output", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1;
	ImGuiListClipper clipper;
	clipper.Begin((int)s_output.size());
	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const compileLine_t &l = s_output[i];
			switch (l.kind)
			{
			case LINE_TASK:		ImGui::TextColored(s_taskColor, "%s", l.text.c_str()); break;
			case LINE_ERROR:	ImGui::TextColored(s_errorColor, "%s", l.text.c_str()); break;
			case LINE_SUCCESS:	ImGui::TextColored(s_successColor, "%s", l.text.c_str()); break;
			default:			ImGui::TextUnformatted(l.text.c_str()); break;
			}
		}
	}
	if (s_scrollOutput || atBottom)
		ImGui::SetScrollHereY(1.0f);
	s_scrollOutput = false;
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::MenuItem("Copy all"))
		{
			std::string all;
			for (const compileLine_t &l : s_output)
				all += l.text + "\n";
			ImGui::SetClipboardText(all.c_str());
		}
		if (ImGui::MenuItem("Clear", nullptr, false, !g_compileRunner.Running()))
		{
			g_compileRunner.ClearLines();
			s_output.clear();
			s_outputTaken = 0;
		}
		ImGui::EndPopup();
	}
	ImGui::EndChild();
}

static void StartRun(const compileProfile_t &profile, const gameConfig_t &game, bool test)
{
	g_compileRunner.ClearLines();
	s_output.clear();
	s_outputTaken = 0;
	s_scrollOutput = true;
	g_compileRunner.Start(profile, game, g_map.filename, test);
}

void WndCompile::Draw()
{
	if (!s_compileOpen)
		return;

	ImGui::SetNextWindowSize(ImVec2(980, 760), ImGuiCond_FirstUseEver);
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x / 2, vp->WorkPos.y + vp->WorkSize.y / 2),
		ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	if (s_compileFocus)
	{
		ImGui::SetNextWindowFocus();
		s_compileFocus = false;
	}

	bool open = true;
	if (!ImGui::Begin("Compile Map###RadiantCompile", &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		s_compileOpen = open;
		return;
	}

	const gameConfig_t *game = GameConfigs::Active();
	gameConfig_t *owner = GameConfigs::CompileOwner();
	bool running = g_compileRunner.Running();

	if (!game || !owner)
	{
		ImGui::TextDisabled(game ? "%s is a mod of no game: its game's compilation profiles compile its maps." :
			"No active game: set one up in Tools > Options.", game ? game->name.c_str() : "");
		if (ImGui::Button("Options..."))
			WndOptions::Open();
	}
	else
	{
		gameConfig_t gameCopy = *game;		// the profile edits below make Active() again
		compileVars_t vars = Compilation::Variables(gameCopy, g_map.filename);
		bool changed = false;

		float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		float outputH = std::max(140.0f, ImGui::GetContentRegionAvail().y * 0.32f);
		float topH = ImGui::GetContentRegionAvail().y - outputH - footer;

		ImGui::BeginDisabled(running);
		ImGui::BeginChild("##left", ImVec2(230, topH));
		changed |= ProfileList(*owner, gameCopy);
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("##right", ImVec2(0, topH));
		if (s_profile >= 0 && s_profile < (int)owner->compileProfiles.size())
			changed |= ProfileEditor(owner->compileProfiles[s_profile], gameCopy, vars, running);
		else
			ImGui::TextDisabled(owner->compileProfiles.empty() ? "Add a profile, or the Defaults for the game's compilation tools." :
				"Pick a profile.");
		ImGui::EndChild();
		ImGui::EndDisabled();
		if (changed)
			GameConfigs::Changed();

		ImGui::SeparatorText("Output");
		ImGui::BeginChild("##outputArea", ImVec2(0, outputH - ImGui::GetFrameHeightWithSpacing()));
		Output();
		ImGui::EndChild();

		// the footer: the map, then trenchbroom's buttons
		ImGui::Separator();
		bool hasMap = !g_map.filename.empty();
		ImGui::AlignTextToFramePadding();
		if (hasMap)
			ImGui::TextDisabled("%s", g_map.filename.c_str());
		else
			ImGui::TextColored(s_errorColor, "no map file: open one to compile it");

		bool sel = s_profile >= 0 && s_profile < (int)owner->compileProfiles.size();
		float bw = 90;
		ImGui::SameLine(ImGui::GetContentRegionMax().x - bw * 4 - ImGui::GetStyle().ItemSpacing.x * 3);
		if (ImGui::Button("Launch...", ImVec2(bw, 0)))
			WndLaunch::Open();
		ImGui::SetItemTooltip("Run > Launch Engine");
		ImGui::SameLine();
		ImGui::BeginDisabled(running || !sel || !hasMap);
		if (ImGui::Button("Test", ImVec2(bw, 0)) && sel)
			StartRun(owner->compileProfiles[s_profile], gameCopy, true);
		ImGui::SetItemTooltip("Prints what the profile would do, without doing it");
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (running)
		{
			if (ImGui::Button("Stop", ImVec2(bw, 0)))
				g_compileRunner.Stop();
		}
		else
		{
			ImGui::BeginDisabled(!sel || !hasMap);
			if (ImGui::Button("Compile", ImVec2(bw, 0)) && sel)
				StartRun(owner->compileProfiles[s_profile], gameCopy, false);
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (ImGui::Button("Close", ImVec2(bw, 0)))
			open = false;
	}

	ImGui::End();
	s_compileOpen = open;
}

/*
==============================================================================

	LAUNCH ENGINE

==============================================================================
*/

static bool		s_launchOpen;
static bool		s_launchFocus;
static std::string	s_launchError;

void WndLaunch::Open()
{
	s_launchOpen = true;
	s_launchFocus = true;
	s_launchError.clear();
}

// the engine profile the active game or mod runs on, to change its parameters in place
static gameConfig_t *ActiveEngine()
{
	const gameConfig_t *c = GameConfigs::Find(GameConfigs::configs, GameConfigs::activeName);
	while (c && c->kind != CONFIG_ENGINE)
		c = GameConfigs::Parent(GameConfigs::configs, *c);
	return c ? &GameConfigs::configs[c - GameConfigs::configs.data()] : nullptr;
}

void WndLaunch::Draw()
{
	if (!s_launchOpen)
		return;

	ImGui::SetNextWindowSize(ImVec2(760, 330), ImGuiCond_FirstUseEver);
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x / 2, vp->WorkPos.y + vp->WorkSize.y / 2),
		ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	if (s_launchFocus)
	{
		ImGui::SetNextWindowFocus();
		s_launchFocus = false;
	}

	bool open = true;
	if (!ImGui::Begin("Launch Engine###RadiantLaunch", &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		s_launchOpen = open;
		return;
	}

	const gameConfig_t *active = GameConfigs::Active();
	gameConfig_t *engine = ActiveEngine();
	if (!active || !engine)
	{
		ImGui::TextDisabled(active ? "%s doesn't run on an engine profile: pick one in Tools > Options." :
			"No active game: set one up in Tools > Options.", active ? active->name.c_str() : "");
	}
	else
	{
		ImGui::Text("%s %s, on %s", GameConfigs::KindName(active->kind), active->name.c_str(), engine->name.c_str());
		ImGui::TextDisabled("%s", engine->executable.empty() ? "no engine program" : GameConfigs::Resolve(engine->executable).c_str());

		ImGui::SeparatorText("Parameters");
		ImGui::SetNextItemWidth(-1);
		if (ImGui::InputTextWithHint("##params", DEFAULT_LAUNCH_PARAMS, &engine->launchParams))
			GameConfigs::Changed();
		ImGui::SetItemTooltip("%s's launch parameters, with ${VARIABLE}s", engine->name.c_str());

		gameConfig_t game = *GameConfigs::Active();
		compileVars_t vars = Compilation::Variables(game, g_map.filename);
		std::string program, parameters, workDir, error;
		if (Compilation::EngineCommand(game, vars, program, parameters, workDir, error))
			ImGui::TextWrapped("%s %s", program.c_str(), parameters.c_str());
		else
			ImGui::TextColored(s_errorColor, "%s", error.c_str());
		if (g_map.filename.empty())
			ImGui::TextColored(s_errorColor, "no map open: ${MAP_BASE_NAME} is empty");

		if (ImGui::Button("Default"))
		{
			engine->launchParams = DEFAULT_LAUNCH_PARAMS;
			GameConfigs::Changed();
		}
		ImGui::SameLine();
		VariablesButton(vars, false);

		if (!s_launchError.empty())
			ImGui::TextColored(s_errorColor, "%s", s_launchError.c_str());

		ImGui::Separator();
		if (ImGui::Button("Configure engines..."))
			WndOptions::Open();
		ImGui::SameLine(ImGui::GetContentRegionMax().x - 90 * 2 - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::Button("Launch", ImVec2(90, 0)))
		{
			s_launchError.clear();
			if (Compilation::LaunchEngine(game, vars, s_launchError))
				open = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Close", ImVec2(90, 0)))
			open = false;
	}

	ImGui::End();
	s_launchOpen = open;
}
