//==============================
//	compilation.cpp
//==============================

#include "core/compile/compilation.h"
#include "core/common/log.h"
#include "core/utils/process.h"

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

CompileRunner	g_compileRunner;

/*
==============================================================================

	VARIABLES

==============================================================================
*/

const Compilation::variableHelp_t Compilation::variableHelp[] =
{
	{ "MAP_BASE_NAME",		"the map's file name without its extension: start" },
	{ "MAP_FULL_NAME",		"the map's file name: start.map" },
	{ "MAP_DIR_PATH",		"the map's folder" },
	{ "WORK_DIR_PATH",		"the profile's working folder (compiling only)" },
	{ "MAPS_DIR_PATH",		"the maps folder the engine loads them from: <mod or game>/maps" },
	{ "GAME_DIR_PATH",		"the game's folder" },
	{ "GAME_NAME",			"the game's folder name: ninjawars" },
	{ "MOD_DIR_PATH",		"the mod's folder, empty for a game" },
	{ "MOD_NAME",			"the mod's folder name, empty for a game" },
	{ "MOD_ARGS",			"-game <mod> for a mod, empty for a game" },
	{ "ENGINE_DIR_PATH",	"the engine's folder (Game Executable Directory)" },
	{ "ENGINE_PATH",		"the engine program" },
	{ "APP_DIR_PATH",		"the editor's folder" },
	{ "CPU_COUNT",			"the number of CPU threads" },
	{ nullptr, nullptr }
};

const char *Compilation::TaskName(int type)
{
	switch (type)
	{
	case TASK_EXPORT_MAP:		return "Export Map";
	case TASK_COPY_FILES:		return "Copy Files";
	case TASK_RENAME_FILE:		return "Rename File";
	case TASK_DELETE_FILES:		return "Delete Files";
	case TASK_RUN_TOOL:			return "Run Tool";
	case TASK_LAUNCH_ENGINE:	return "Launch Engine";
	default:					return "?";
	}
}

compileVars_t Compilation::Variables(const gameConfig_t &game, const std::string &mapPath)
{
	compileVars_t v;
	if (!mapPath.empty())
	{
		std::error_code ec;
		fs::path p = fs::absolute(fs::u8path(mapPath), ec).lexically_normal();
		v["MAP_BASE_NAME"] = p.stem().u8string();
		v["MAP_FULL_NAME"] = p.filename().u8string();
		v["MAP_DIR_PATH"] = p.parent_path().u8string();
	}
	else
		v["MAP_BASE_NAME"] = v["MAP_FULL_NAME"] = v["MAP_DIR_PATH"] = std::string();

	std::string gameDir = GameConfigs::Resolve(game.gameDir);
	std::string modDir = GameConfigs::Resolve(game.modDir);
	v["GAME_DIR_PATH"] = gameDir;
	v["GAME_NAME"] = gameDir.empty() ? std::string() : fs::u8path(gameDir).filename().u8string();
	v["MOD_DIR_PATH"] = modDir;
	v["MOD_NAME"] = modDir.empty() ? std::string() : fs::u8path(modDir).filename().u8string();
	v["MOD_ARGS"] = modDir.empty() ? std::string() : "-game " + v["MOD_NAME"];

	std::vector<std::string> dirs = GameConfigs::SearchPaths(game);
	v["MAPS_DIR_PATH"] = dirs.empty() ? std::string() : (fs::u8path(dirs.front()) / "maps").u8string();
	v["ENGINE_DIR_PATH"] = GameConfigs::Resolve(game.executableDir);
	v["ENGINE_PATH"] = GameConfigs::Resolve(game.executable);
	v["APP_DIR_PATH"] = GameConfigs::Resolve(".");
	v["CPU_COUNT"] = std::to_string(std::max(1u, std::thread::hardware_concurrency()));

	for (const compileTool_t &t : game.tools)
		v[t.name] = GameConfigs::Resolve(t.path);
	return v;
}

bool Compilation::Interpolate(const std::string &spec, const compileVars_t &vars, std::string &out, std::string &error)
{
	out.clear();
	for (size_t i = 0; i < spec.size(); )
	{
		if (spec.compare(i, 2, "${"))
		{
			out += spec[i++];
			continue;
		}
		size_t end = spec.find('}', i + 2);
		if (end == std::string::npos)
		{
			error = "a ${ with no } in '" + spec + "'";
			return false;
		}
		std::string name = spec.substr(i + 2, end - i - 2);
		auto it = vars.find(name);
		if (it == vars.end())
		{
			error = "no variable ${" + name + "}";
			return false;
		}
		out += it->second;
		i = end + 1;
	}
	return true;
}

bool Compilation::EngineCommand(const gameConfig_t &game, const compileVars_t &vars,
	std::string &program, std::string &parameters, std::string &workDir, std::string &error)
{
	if (game.executable.empty())
	{
		error = "no engine program in " + game.name + "'s engine profile (Tools > Options)";
		return false;
	}
	program = GameConfigs::Resolve(game.executable);
	workDir = game.executableDir.empty() ? fs::u8path(program).parent_path().u8string() : GameConfigs::Resolve(game.executableDir);
	return Interpolate(game.launchParams, vars, parameters, error);
}

bool Compilation::LaunchEngine(const gameConfig_t &game, const compileVars_t &vars, std::string &error)
{
	std::string program, parameters, workDir;
	if (!EngineCommand(game, vars, program, parameters, workDir, error))
		return false;
	Log::Info("Launching %s %s", program.c_str(), parameters.c_str());
	return Process::Launch(program, parameters, workDir, error);
}

/*
==============================================================================

	RUNNER

==============================================================================
*/

CompileRunner::~CompileRunner()
{
	Stop();
}

bool CompileRunner::Start(const compileProfile_t &profile, const gameConfig_t &game, const std::string &mapPath, bool test)
{
	if (running)
		return false;
	if (thread.joinable())
		thread.join();

	cancel = false;
	running = true;
	thread = std::thread(&CompileRunner::Run, this, profile, game, mapPath, test);
	return true;
}

void CompileRunner::Stop()
{
	cancel = true;
	if (thread.joinable())
		thread.join();
	running = false;
}

size_t CompileRunner::TakeLines(std::vector<compileLine_t> &out, size_t from)
{
	std::lock_guard<std::mutex> lock(linesMutex);
	for (size_t i = from; i < lines.size(); i++)
		out.push_back(lines[i]);
	return lines.size();
}

void CompileRunner::ClearLines()
{
	std::lock_guard<std::mutex> lock(linesMutex);
	lines.clear();
}

void CompileRunner::Print(int kind, const std::string &text)
{
	{
		std::lock_guard<std::mutex> lock(linesMutex);
		lines.push_back({ kind, text });
	}
	if (kind == LINE_ERROR)
		Log::Error("%s", text.c_str());
	else if (kind != LINE_OUTPUT)
		Log::Info("%s", text.c_str());
}

// trenchbroom's makeAbsolute: relative to the working folder
static fs::path Absolute(const std::string &path, const compileVars_t &vars)
{
	fs::path p = fs::u8path(path);
	if (!p.is_absolute())
		p = fs::u8path(vars.at("WORK_DIR_PATH")) / p;
	return p.lexically_normal();
}

// a file name pattern, * and ?, case insensitive like the file systems it's for
static bool Matches(const char *pattern, const char *name)
{
	for (; *pattern; pattern++, name++)
	{
		if (*pattern == '*')
		{
			for (const char *n = name; ; n++)
			{
				if (Matches(pattern + 1, n))
					return true;
				if (!*n)
					return false;
			}
		}
		if (!*name || (*pattern != '?' && tolower((unsigned char)*pattern) != tolower((unsigned char)*name)))
			return false;
	}
	return !*name;
}

// the files in pattern's folder whose name matches it
static std::vector<fs::path> FindFiles(const fs::path &pattern)
{
	std::vector<fs::path> out;
	std::error_code ec;
	std::string name = pattern.filename().u8string();
	for (const fs::directory_entry &e : fs::directory_iterator(pattern.parent_path(), ec))
		if (e.is_regular_file(ec) && Matches(name.c_str(), e.path().filename().u8string().c_str()))
			out.push_back(e.path());
	std::sort(out.begin(), out.end());
	return out;
}

static bool SamePath(const fs::path &a, const fs::path &b)
{
	std::error_code ec;
	return fs::equivalent(a, b, ec);
}

/*
==================
CompileRunner::RunTask

one task, after trenchbroom's Compilation*TaskRunner. false stops the profile
==================
*/
bool CompileRunner::RunTask(const compileTask_t &task, const compileVars_t &vars, const gameConfig_t &game, bool test)
{
	std::string a, b, error;
	bool hasB = task.type == TASK_COPY_FILES || task.type == TASK_RENAME_FILE || task.type == TASK_RUN_TOOL;
	if (task.type != TASK_LAUNCH_ENGINE)
	{
		if (!Compilation::Interpolate(task.a, vars, a, error) || (hasB && !Compilation::Interpolate(task.b, vars, b, error)))
		{
			Print(LINE_ERROR, std::string("#### ") + Compilation::TaskName(task.type) + ": " + error);
			return false;
		}
	}
	std::error_code ec;

	switch (task.type)
	{
	case TASK_EXPORT_MAP:
	{
		fs::path target = Absolute(a, vars);
		fs::path source = fs::u8path(vars.at("MAP_DIR_PATH")) / fs::u8path(vars.at("MAP_FULL_NAME"));
		Print(LINE_TASK, "#### Exporting map file '" + target.u8string() + "'");
		if (test)
			return true;
		if (SamePath(source, target))
			return true;
		fs::create_directories(target.parent_path(), ec);
		if (!fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec))
		{
			Print(LINE_ERROR, "#### Export failed: " + ec.message());
			return false;
		}
		return true;
	}

	case TASK_COPY_FILES:
	{
		fs::path source = Absolute(a, vars), target = Absolute(b, vars);
		std::vector<fs::path> files = FindFiles(source);
		std::string list;
		for (const fs::path &f : files)
			list += (list.empty() ? "" : ", ") + f.filename().u8string();
		Print(LINE_TASK, "#### Copying to '" + target.u8string() + "': " + (list.empty() ? std::string("no files") : list));
		if (test)
			return true;
		fs::create_directories(target, ec);
		for (const fs::path &f : files)
		{
			if (SamePath(f.parent_path(), target))
				continue;	// it's there already
			if (!fs::copy_file(f, target / f.filename(), fs::copy_options::overwrite_existing, ec))
			{
				Print(LINE_ERROR, "#### Copy failed: " + f.u8string() + ": " + ec.message());
				return false;
			}
		}
		return true;
	}

	case TASK_RENAME_FILE:
	{
		fs::path source = Absolute(a, vars), target = Absolute(b, vars);
		Print(LINE_TASK, "#### Renaming '" + source.u8string() + "' to '" + target.u8string() + "'");
		if (test)
			return true;
		fs::create_directories(target.parent_path(), ec);
		if (fs::exists(target, ec) && !SamePath(source, target))
			fs::remove(target, ec);
		fs::rename(source, target, ec);
		if (ec)
		{
			Print(LINE_ERROR, "#### Rename failed: " + ec.message());
			return false;
		}
		return true;
	}

	case TASK_DELETE_FILES:
	{
		std::vector<fs::path> files = FindFiles(Absolute(a, vars));
		std::string list;
		for (const fs::path &f : files)
			list += (list.empty() ? "" : ", ") + f.u8string();
		Print(LINE_TASK, "#### Deleting: " + (list.empty() ? std::string("no files") : list));
		if (test)
			return true;
		for (const fs::path &f : files)
		{
			if (!fs::remove(f, ec) && ec)
			{
				Print(LINE_ERROR, "#### Delete failed: " + f.u8string() + ": " + ec.message());
				return false;
			}
		}
		return true;
	}

	case TASK_RUN_TOOL:
	{
		Print(LINE_TASK, "#### Executing '" + a + " " + b + "'");
		if (test)
			return true;
		if (a.empty())
		{
			Print(LINE_ERROR, "#### No tool: is its path set in Tools > Options?");
			return false;
		}
		int code = Process::Run(a, b, vars.at("WORK_DIR_PATH"),
			[this](const std::string &line) { Print(LINE_OUTPUT, line); }, cancel, error);
		if (cancel)
		{
			Print(LINE_ERROR, "#### Terminated");
			return false;
		}
		if (code < 0)
		{
			Print(LINE_ERROR, "#### Couldn't run it: " + error);
			return false;
		}
		if (code != 0)
		{
			Print(task.flag ? LINE_ERROR : LINE_TASK, "#### Finished with exit code " + std::to_string(code));
			return !task.flag;
		}
		Print(LINE_TASK, "#### Finished with exit code 0");
		return true;
	}

	case TASK_LAUNCH_ENGINE:
	{
		std::string program, parameters, workDir;
		if (!Compilation::EngineCommand(game, vars, program, parameters, workDir, error))
		{
			Print(task.flag ? LINE_ERROR : LINE_TASK, "#### Launch failed: " + error);
			return !task.flag;
		}
		Print(LINE_TASK, "#### Launching '" + program + " " + parameters + "'");
		if (test)
			return true;
		if (!Process::Launch(program, parameters, workDir, error))
		{
			Print(task.flag ? LINE_ERROR : LINE_TASK, "#### Launch failed: " + error);
			return !task.flag;
		}
		return true;
	}
	}
	return true;
}

void CompileRunner::Run(compileProfile_t profile, gameConfig_t game, std::string mapPath, bool test)
{
	auto start = std::chrono::steady_clock::now();
	Print(LINE_TASK, "#### " + std::string(test ? "Testing" : "Running") + " compilation profile '" + profile.name + "'");

	compileVars_t vars = Compilation::Variables(game, mapPath);
	std::string workDir, error;
	bool ok = true;
	if (!Compilation::Interpolate(profile.workDir, vars, workDir, error))
	{
		Print(LINE_ERROR, "#### Working folder: " + error);
		ok = false;
	}
	else
	{
		vars["WORK_DIR_PATH"] = fs::u8path(workDir).lexically_normal().u8string();
		std::error_code ec;
		if (!test && !workDir.empty())
			fs::create_directories(fs::u8path(workDir), ec);
		Print(LINE_TASK, "#### Working folder '" + vars["WORK_DIR_PATH"] + "'");
	}

	for (size_t i = 0; ok && i < profile.tasks.size(); i++)
	{
		if (!profile.tasks[i].enabled)
			continue;
		if (cancel)
		{
			ok = false;
			break;
		}
		currentTask = (int)i;
		ok = RunTask(profile.tasks[i], vars, game, test);
	}
	currentTask = -1;

	double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	char took[64];
	std::snprintf(took, sizeof(took), "%.1f s", secs);
	if (ok)
		Print(LINE_SUCCESS, std::string("#### ") + (test ? "Test finished" : "Finished") + " in " + took);
	else
		Print(LINE_ERROR, std::string("#### ") + (cancel ? "Stopped" : "Failed") + " after " + took);
	running = false;
}
