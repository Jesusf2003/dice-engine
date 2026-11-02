//==============================
//	compilation.h
//
//	compiling maps and launching the engine, after trenchbroom's
//	CompilationVariables, CompilationRunner and LaunchGameEngine:
//
//	-	the specs of a compilation profile and the engine's launch parameters
//		are text with ${VARIABLE}s: the map's name and folders, the game's and
//		mod's, the engine's, the CPU count, and every compilation tool by its
//		name (${qbsp} is qbsp's path)
//	-	CompileRunner runs a profile's enabled tasks in order on a thread of its
//		own, collecting what they print; the first that fails stops the rest.
//		a test run only prints what would be done
//
//	the editor can't save maps yet, so Export Map copies the map file as it is
//	on disk
//==============================

#ifndef __COMPILATION_H__
#define __COMPILATION_H__

#include "core/common/gameConfig.h"

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

typedef std::map<std::string, std::string>	compileVars_t;

namespace Compilation
{
	struct variableHelp_t
	{
		const char	*name;
		const char	*help;
	};
	extern const variableHelp_t	variableHelp[];		// ends with a null name

	const char	*TaskName(int type);	// "Run Tool"...

	// for game, the active profile with what it inherits, and the map at mapPath (empty: none)
	compileVars_t	Variables(const gameConfig_t &game, const std::string &mapPath);
	// spec with its ${VARIABLE}s filled in; false and error on one that isn't there
	bool	Interpolate(const std::string &spec, const compileVars_t &vars, std::string &out, std::string &error);

	// the engine's program, parameters and folder, filled in
	bool	EngineCommand(const gameConfig_t &game, const compileVars_t &vars,
				std::string &program, std::string &parameters, std::string &workDir, std::string &error);
	bool	LaunchEngine(const gameConfig_t &game, const compileVars_t &vars, std::string &error);
}

enum compileLineKind_t
{
	LINE_OUTPUT,		// what a tool printed
	LINE_TASK,			// trenchbroom's "#### ..." lines
	LINE_ERROR,
	LINE_SUCCESS
};

struct compileLine_t
{
	int			kind;
	std::string	text;
};

class CompileRunner
{
public:
	~CompileRunner();

	// false when a run is going on
	bool	Start(const compileProfile_t &profile, const gameConfig_t &game, const std::string &mapPath, bool test);
	void	Stop();
	bool	Running() const		{ return running; }
	int		CurrentTask() const	{ return currentTask; }	// -1 when none

	// the lines from the from-th on, appended to out; the count there is
	size_t	TakeLines(std::vector<compileLine_t> &out, size_t from);
	void	ClearLines();

private:
	void	Run(compileProfile_t profile, gameConfig_t game, std::string mapPath, bool test);
	bool	RunTask(const compileTask_t &task, const compileVars_t &vars, const gameConfig_t &game, bool test);
	void	Print(int kind, const std::string &text);

	std::thread					thread;
	std::atomic<bool>			running{ false }, cancel{ false };
	std::atomic<int>			currentTask{ -1 };
	std::mutex					linesMutex;
	std::vector<compileLine_t>	lines;
};

extern CompileRunner	g_compileRunner;

#endif
