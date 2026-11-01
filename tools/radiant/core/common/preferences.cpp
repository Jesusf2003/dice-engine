//==============================
//	preferences.cpp
//==============================

#include "core/common/preferences.h"
#include "core/common/log.h"

#include <imgui.h>
#include <imgui_internal.h>	// ImGuiSettingsHandler

#include <algorithm>
#include <cstdlib>
#include <cstring>

preferences_t g_prefs;

enum { PAGE_CONSOLE = 1, PAGE_CONSOLE_ADVANCED };

/*
==============================================================================

	RADIANT.INI

	[RadiantPrefs][Console] and [RadiantPrefs][ConsoleAdvanced], key=value
	per line. unknown keys are skipped so old and new versions can share a file
==============================================================================
*/

static bool ParseBool(const char *v)	{ return atoi(v) != 0; }

static void *Prefs_ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
	if (!strcmp(name, "Console"))
		return (void *)(intptr_t)PAGE_CONSOLE;
	if (!strcmp(name, "ConsoleAdvanced"))
		return (void *)(intptr_t)PAGE_CONSOLE_ADVANCED;
	return nullptr;
}

static void Prefs_ReadLine(ImGuiContext *, ImGuiSettingsHandler *, void *entry, const char *line)
{
	const char *eq = strchr(line, '=');
	if (!eq)
		return;
	std::string key(line, eq);
	const char *v = eq + 1;

	if ((intptr_t)entry == PAGE_CONSOLE)
	{
		consolePrefs_t &c = g_prefs.console;
		if (key == "Scrollback")		c.scrollback = std::clamp(atoi(v), 100, 1000000);
		else if (key == "Timestamps")	c.timestamps = ParseBool(v);
		else if (key == "AutoScroll")	c.autoScroll = ParseBool(v);
		else if (key == "ShowOnError")	c.showOnError = ParseBool(v);
	}
	else
	{
		consoleAdvancedPrefs_t &a = g_prefs.consoleAdvanced;
		if (key == "LogDetail")				a.logDetail = std::clamp(atoi(v), 0, MAPLOG_COUNT - 1);
		else if (key == "WarningLimit")		a.warningLimit = std::max(atoi(v), 0);
		else if (key == "Timings")			a.timings = ParseBool(v);
		else if (key == "CaptureSDL")		a.captureSDL = ParseBool(v);
		else if (key == "LogToFile")		a.logToFile = ParseBool(v);
		else if (key == "LogFile")			a.logFile = v;
		else if (key == "AppendLogFile")	a.appendLogFile = ParseBool(v);
		else if (key == "ValidateGeometry")	a.validateGeometry = ParseBool(v);
		else if (key == "ValidateTextures")	a.validateTextures = ParseBool(v);
		else if (key == "ValidateTargets")	a.validateTargets = ParseBool(v);
	}
}

static void Prefs_ApplyAll(ImGuiContext *, ImGuiSettingsHandler *)
{
	Log::PrefsChanged();
}

static void Prefs_WriteAll(ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf)
{
	const consolePrefs_t &c = g_prefs.console;
	const consoleAdvancedPrefs_t &a = g_prefs.consoleAdvanced;

	buf->appendf("[%s][Console]\n", handler->TypeName);
	buf->appendf("Scrollback=%d\n", c.scrollback);
	buf->appendf("Timestamps=%d\n", c.timestamps);
	buf->appendf("AutoScroll=%d\n", c.autoScroll);
	buf->appendf("ShowOnError=%d\n", c.showOnError);
	buf->append("\n");

	buf->appendf("[%s][ConsoleAdvanced]\n", handler->TypeName);
	buf->appendf("LogDetail=%d\n", a.logDetail);
	buf->appendf("WarningLimit=%d\n", a.warningLimit);
	buf->appendf("Timings=%d\n", a.timings);
	buf->appendf("CaptureSDL=%d\n", a.captureSDL);
	buf->appendf("LogToFile=%d\n", a.logToFile);
	buf->appendf("LogFile=%s\n", a.logFile.c_str());
	buf->appendf("AppendLogFile=%d\n", a.appendLogFile);
	buf->appendf("ValidateGeometry=%d\n", a.validateGeometry);
	buf->appendf("ValidateTextures=%d\n", a.validateTextures);
	buf->appendf("ValidateTargets=%d\n", a.validateTargets);
	buf->append("\n");
}

void Preferences::RegisterSettings()
{
	ImGuiSettingsHandler handler;
	handler.TypeName = "RadiantPrefs";
	handler.TypeHash = ImHashStr("RadiantPrefs");
	handler.ReadOpenFn = Prefs_ReadOpen;
	handler.ReadLineFn = Prefs_ReadLine;
	handler.ApplyAllFn = Prefs_ApplyAll;
	handler.WriteAllFn = Prefs_WriteAll;
	ImGui::AddSettingsHandler(&handler);
}

void Preferences::Apply(const preferences_t &prefs)
{
	g_prefs = prefs;
	Log::PrefsChanged();
	ImGui::MarkIniSettingsDirty();
}
