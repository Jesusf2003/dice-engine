//==============================
//	wndPreferences.cpp
//==============================

#include "ui/preferences/wndPreferences.h"
#include "core/common/preferences.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>

enum { PAGE_CONSOLE, PAGE_CONSOLE_ADVANCED };

static bool				s_open;
static bool				s_wantFocus;
static int				s_page = PAGE_CONSOLE;
static preferences_t	s_edit;		// what the window shows, g_prefs once applied

void WndPreferences::Open()
{
	if (!s_open)
		s_edit = g_prefs;
	s_open = true;
	s_wantFocus = true;
}

static void HelpMarker(const char *text)
{
	ImGui::SameLine();
	ImGui::TextDisabled("(?)");
	ImGui::SetItemTooltip("%s", text);
}

/*
==============================================================================

	PAGES

==============================================================================
*/

static void PageConsole(consolePrefs_t &c)
{
	ImGui::SeparatorText("Output");

	ImGui::SetNextItemWidth(160);
	if (ImGui::InputInt("Scrollback lines", &c.scrollback, 1000, 10000))
		c.scrollback = std::clamp(c.scrollback, 100, 1000000);
	HelpMarker("How many lines the console keeps, the oldest drop off.");

	ImGui::Checkbox("Timestamps", &c.timestamps);
	ImGui::Checkbox("Follow new lines", &c.autoScroll);
	HelpMarker("Scroll to new lines as they come, while the console is already at the bottom.");
	ImGui::Checkbox("Show the console when an error is logged", &c.showOnError);
}

static void PageConsoleAdvanced(consoleAdvancedPrefs_t &a)
{
	static const char *details[MAPLOG_COUNT] = {
		"Summary",
		"Entities (verbose)",
		"Brushes and faces (debug)",
	};

	ImGui::SeparatorText("Detail");

	ImGui::SetNextItemWidth(220);
	ImGui::Combo("Log detail", &a.logDetail, details, MAPLOG_COUNT);
	HelpMarker("Summary: the load report and every problem found.\n"
		"Entities: plus a line per entity and where each wad was found.\n"
		"Brushes and faces: plus every brush with its bounds and every face with its plane.\n"
		"A big map at the last level is thousands of lines.");

	ImGui::SetNextItemWidth(160);
	if (ImGui::InputInt("Warning limit per kind", &a.warningLimit, 5, 50))
		a.warningLimit = std::max(a.warningLimit, 0);
	HelpMarker("After this many warnings of the same kind in one map load (\"missing texture\", "
		"\"absolute wad path\"...) the rest are only counted. 0 shows them all. Errors always show.");

	ImGui::Checkbox("Log timings", &a.timings);
	HelpMarker("How long parsing and validating a map took.");

	ImGui::SeparatorText("Sources");
	ImGui::Checkbox("Capture SDL messages", &a.captureSDL);
	HelpMarker("SDL_Log output (texture loading, OpenGL) shows in the console too.");

	ImGui::SeparatorText("Log file");
	ImGui::Checkbox("Write the log to a file", &a.logToFile);
	ImGui::BeginDisabled(!a.logToFile);
	ImGui::SetNextItemWidth(320);
	ImGui::InputText("File", &a.logFile);
	HelpMarker("Relative to the working directory. Opening it writes what the console already has.");
	ImGui::Checkbox("Append (keep earlier sessions)", &a.appendLogFile);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Map load analysis");
	ImGui::Checkbox("Check brush geometry", &a.validateGeometry);
	HelpMarker("Builds every brush from its planes like qbsp does: degenerate, repeated and "
		"redundant faces, brushes that enclose nothing, brushes out of bounds.");
	ImGui::Checkbox("Check textures against the loaded wads", &a.validateTextures);
	ImGui::Checkbox("Check target / killtarget", &a.validateTargets);
	HelpMarker("Targets that no entity has as its targetname.");
}

/*
==================
WndPreferences::Draw
==================
*/
void WndPreferences::Draw()
{
	if (!s_open)
		return;

	ImGui::SetNextWindowSize(ImVec2(720, 460), ImGuiCond_FirstUseEver);
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x / 2, vp->WorkPos.y + vp->WorkSize.y / 2),
		ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	if (s_wantFocus)
	{
		ImGui::SetNextWindowFocus();
		s_wantFocus = false;
	}

	bool open = true;
	if (!ImGui::Begin("Preferences###RadiantPreferences", &open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		s_open = open;
		return;
	}

	float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;

	// the page tree
	ImGui::BeginChild("##pages", ImVec2(180, -footer), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
	ImGuiTreeNodeFlags base = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	bool consoleOpen = ImGui::TreeNodeEx("Console", base | (s_page == PAGE_CONSOLE ? ImGuiTreeNodeFlags_Selected : 0));
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		s_page = PAGE_CONSOLE;
	if (consoleOpen)
	{
		ImGui::TreeNodeEx("Advanced", base | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
			(s_page == PAGE_CONSOLE_ADVANCED ? ImGuiTreeNodeFlags_Selected : 0));
		if (ImGui::IsItemClicked())
			s_page = PAGE_CONSOLE_ADVANCED;
		ImGui::TreePop();
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("##page", ImVec2(0, -footer));
	switch (s_page)
	{
	case PAGE_CONSOLE:			PageConsole(s_edit.console); break;
	case PAGE_CONSOLE_ADVANCED:	PageConsoleAdvanced(s_edit.consoleAdvanced); break;
	}
	ImGui::EndChild();

	// buttons
	ImGui::Separator();
	if (ImGui::Button("Defaults"))
	{
		if (s_page == PAGE_CONSOLE)
			s_edit.console = consolePrefs_t();
		else
			s_edit.consoleAdvanced = consoleAdvancedPrefs_t();
	}
	ImGui::SetItemTooltip("Put this page back to the defaults (applied with OK or Apply)");

	float bw = 80;
	ImGui::SameLine(ImGui::GetContentRegionMax().x - bw * 3 - ImGui::GetStyle().ItemSpacing.x * 2);
	if (ImGui::Button("OK", ImVec2(bw, 0)))
	{
		Preferences::Apply(s_edit);
		open = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Apply", ImVec2(bw, 0)))
		Preferences::Apply(s_edit);
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(bw, 0)) || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape)))
		open = false;

	ImGui::End();
	s_open = open;
}
