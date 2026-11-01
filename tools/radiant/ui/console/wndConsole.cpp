//==============================
//	wndConsole.cpp
//==============================

#include "ui/console/wndConsole.h"
#include "ui/wndMain.h"
#include "core/common/preferences.h"

#include <string>

static const ImVec4 s_levelColors[LOG_LEVELS] = {
	ImVec4(1.00f, 0.40f, 0.40f, 1.0f),	// error
	ImVec4(1.00f, 0.80f, 0.30f, 1.0f),	// warning
	ImVec4(0.86f, 0.86f, 0.86f, 1.0f),	// info
	ImVec4(0.60f, 0.70f, 0.80f, 1.0f),	// verbose
	ImVec4(0.55f, 0.55f, 0.55f, 1.0f),	// debug
};
static const ImVec4 s_timeColor(0.50f, 0.50f, 0.50f, 1.0f);
static const char *s_levelLabels[LOG_LEVELS] = { "Errors", "Warnings", "Info", "Verbose", "Debug" };

WndConsole::WndConsole() :
	indexedGeneration(0), indexDirty(true), scrollToEnd(true)
{
	name = "Console";		// keeps the id ("###Console0") the layouts dock by
	vbits = W_CONSOLE;
	SetTitle("Console");
	for (bool &b : showLevel)
		b = true;
}

void WndConsole::ScrollToEnd()
{
	scrollToEnd = true;
}

/*
==================
WndConsole::RebuildIndex

which log lines pass the level toggles and the text filter
==================
*/
void WndConsole::RebuildIndex()
{
	size_t n = Log::Count();

	visible.clear();
	visible.reserve(n);
	for (size_t i = 0; i < n; i++)
	{
		const Log::line_t &l = Log::Line(i);
		if (showLevel[l.level] && filter.PassFilter(l.text.c_str()))
			visible.push_back(i);
	}
	indexedGeneration = Log::Generation();
	indexDirty = false;
}

void WndConsole::CopyToClipboard()
{
	std::string out;
	for (size_t i : visible)
	{
		const Log::line_t &l = Log::Line(i);
		if (g_prefs.console.timestamps)
			out += "[" + Log::FormatTime(l.timeMs) + "] ";
		out += l.text;
		out += '\n';
	}
	ImGui::SetClipboardText(out.c_str());
}

void WndConsole::DrawToolbar()
{
	if (ImGui::Button("Clear"))
		Log::Clear();
	ImGui::SameLine();
	if (ImGui::Button("Copy"))
		CopyToClipboard();
	ImGui::SetItemTooltip("Copy the lines shown");

	const ImGuiStyle &style = ImGui::GetStyle();
	for (int i = 0; i < LOG_LEVELS; i++)
	{
		// wrap when the inspector is too narrow for the whole row
		float w = ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize(s_levelLabels[i]).x;
		ImGui::SameLine();
		if (ImGui::GetContentRegionAvail().x < w)
			ImGui::NewLine();
		ImGui::PushStyleColor(ImGuiCol_Text, s_levelColors[i]);
		if (ImGui::Checkbox(s_levelLabels[i], &showLevel[i]))
			indexDirty = true;
		ImGui::PopStyleColor();
		// the detail levels only get logged when Console > Advanced asks for them
		if (i >= LOG_VERBOSE && !Log::Enabled((LogLevel)i))
			ImGui::SetItemTooltip("Not being logged: Edit > Preferences > Console > Advanced, log detail");
	}

	// a row of its own, full width
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputTextWithHint("##filter", "filter: words, -exclude, comma separated", filter.InputBuf, IM_ARRAYSIZE(filter.InputBuf)))
	{
		filter.Build();
		indexDirty = true;
	}
}

/*
==================
WndConsole::OnPaint
==================
*/
int WndConsole::OnPaint()
{
	const consolePrefs_t &prefs = g_prefs.console;

	DrawToolbar();

	if (indexDirty || indexedGeneration != Log::Generation())
		RebuildIndex();

	ImGui::BeginChild("##log", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

	// following the end: only when the view was already there (or asked to)
	bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	ImGuiListClipper clipper;
	clipper.Begin((int)visible.size());
	while (clipper.Step())
	{
		for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; r++)
		{
			const Log::line_t &l = Log::Line(visible[r]);
			if (prefs.timestamps)
			{
				ImGui::TextColored(s_timeColor, "%s", Log::FormatTime(l.timeMs).c_str());
				ImGui::SameLine();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, s_levelColors[l.level]);
			ImGui::TextUnformatted(l.text.c_str(), l.text.c_str() + l.text.size());
			ImGui::PopStyleColor();
		}
	}
	clipper.End();
	ImGui::PopStyleVar();

	if (ImGui::BeginPopupContextWindow("##logmenu"))
	{
		if (ImGui::MenuItem("Copy"))
			CopyToClipboard();
		if (ImGui::MenuItem("Clear"))
			Log::Clear();
		ImGui::Separator();
		if (ImGui::MenuItem("Timestamps", nullptr, g_prefs.console.timestamps))
		{
			preferences_t p = g_prefs;
			p.console.timestamps = !p.console.timestamps;
			Preferences::Apply(p);
		}
		ImGui::EndPopup();
	}

	if (scrollToEnd || (prefs.autoScroll && atBottom))
		ImGui::SetScrollHereY(1.0f);
	scrollToEnd = false;

	ImGui::EndChild();
	return 1;
}
