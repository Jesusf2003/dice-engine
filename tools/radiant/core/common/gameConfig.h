//==============================
//	gameConfig.h
//
//	the games and mods the editor can map for, hammer's "Game Configurations"
//	(Tools > Options), as profiles of three kinds that build on each other:
//
//	-	engine	the engine's folder and executable, how to launch it, and the
//				compilation tools (qbsp, vis, light...) that make its maps
//	-	game	a base game on an engine: its folder (like quake's id1), its game
//				data files (FGDs), where its maps are, the compilation profiles
//				that build them (core/compile/compilation.h); tools of its own
//	-	mod		a mod of a game: a folder searched before the game's, FGDs loaded
//				after the game's
//
//	a profile names the one it builds on (a game its engine, a mod its game) and
//	takes what it leaves empty from it, FGDs being added after the parent's and
//	tools replacing the parent's of the same name. a mod compiles its maps with
//	its game's compilation profiles.
//	a game or a mod can be the active profile, the one the editor maps for;
//	Active() hands out that profile with everything it inherits filled in.
//	the wads aren't here, they're in the FGD's wadlist, relative to the folders.
//	stored in radiant.ini as [RadiantGames][GameN] entries.
//
//	paths can be relative to the editor's folder, so a profile keeps working
//	when the whole bin folder moves
//==============================

#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

#include <string>
#include <vector>

/*
	trenchbroom's compilation model (CompilationTask.h, CompilationProfile.h):
	a profile is a working folder and tasks run in order. the specs are text
	with ${VARIABLE}s in them, core/compile/compilation.h fills them in
*/
enum compileTaskType_t
{
	TASK_EXPORT_MAP,		// target: the map written there
	TASK_COPY_FILES,		// source (a file name pattern, * and ?) to target, a folder
	TASK_RENAME_FILE,		// source to target
	TASK_DELETE_FILES,		// target, a file name pattern
	TASK_RUN_TOOL,			// tool, parameters; flag: a result other than 0 is an error
	TASK_LAUNCH_ENGINE,		// the engine with its launch parameters; flag: not starting is an error
	TASK_TYPE_COUNT
};

struct compileTask_t
{
	int			type = TASK_RUN_TOOL;
	bool		enabled = true;
	bool		flag = true;
	std::string	a, b;		// export: target; copy, rename: source, target; delete: target; run: tool, parameters
};

struct compileProfile_t
{
	std::string	name;
	std::string	workDir = "${MAP_DIR_PATH}";
	std::vector<compileTask_t>	tasks;
};

// a compilation tool: ${name} in the specs is its path
struct compileTool_t
{
	std::string	name;
	std::string	path;
};

enum gameConfigKind_t
{
	CONFIG_ENGINE,
	CONFIG_GAME,
	CONFIG_MOD,
	CONFIG_KIND_COUNT
};

struct gameConfig_t
{
	std::string	name;
	int			kind = CONFIG_GAME;
	std::string	parent;					// a game's engine, a mod's game; empty: stands alone

	std::vector<std::string>	fgds;	// game data files, loaded in order (the parent's first): a later one redefines classes

	std::vector<compileTool_t>	tools;	// engine, game: a game's replace its engine's of the same name

	// engine
	std::string	executableDir;			// where the engine is, its -basedir
	std::string	executable;				// the engine program: fteqw64.exe
	std::string	launchParams;			// trenchbroom's engine profile parameters, with ${VARIABLE}s

	// game, mod
	std::string	gameDir;				// the base game's folder: ninjawars, like quake's id1
	std::string	modDir;					// a mod's folder, searched before gameDir
	std::string	mapsDir;				// File > Open starts here; empty: <mod or game>/maps. not inherited
	std::string	defaultPointClass;		// hammer's defaults for new entities (used once entities can be placed)
	std::string	defaultSolidClass;
	std::vector<compileProfile_t>	compileProfiles;	// game: Run > Compile Map. not inherited, a mod uses its game's
};

#define DEFAULT_LAUNCH_PARAMS	"-basedir \"${ENGINE_DIR_PATH}\" -nohome -window +set sv_public 0 ${MOD_ARGS} +map ${MAP_BASE_NAME}"

namespace GameConfigs
{
	extern std::vector<gameConfig_t>	configs;
	extern std::string					activeName;	// a game or a mod

	const gameConfig_t	*Active();		// with what it inherits; nullptr when there's none
	void	Apply(const std::vector<gameConfig_t> &list, const std::string &active);	// marks radiant.ini for saving
	// the game whose compilation profiles the active game or mod uses, to change them in place
	gameConfig_t	*CompileOwner();
	void	Changed();				// after changing configs in place: Active() again, radiant.ini saved

	const char	*KindName(int kind);	// "Engine", "Game", "Mod"
	int		ParentKind(int kind);		// what a profile of that kind builds on, -1 for an engine
	bool	CanBeActive(int kind);
	const gameConfig_t	*Find(const std::vector<gameConfig_t> &list, const std::string &name);
	const gameConfig_t	*Parent(const std::vector<gameConfig_t> &list, const gameConfig_t &cfg);	// nullptr when missing or of the wrong kind
	// the profile with everything it inherits from its parents filled in
	gameConfig_t	Inherited(const std::vector<gameConfig_t> &list, const gameConfig_t &cfg);

	std::string	EditorDir();						// the folder the editor runs from
	std::string	Resolve(const std::string &path);	// relative paths are from EditorDir
	std::string	MakeRelative(const std::string &path);	// inside EditorDir: relative, so it travels with it

	// of a resolved profile
	std::vector<std::string>	SearchPaths(const gameConfig_t &cfg);	// absolute: mod, then game
	std::string	MapsDir(const gameConfig_t &cfg);

	const compileTool_t	*FindTool(const gameConfig_t &cfg, const std::string &name);
	// the tools next to the editor (qbsp, vis, light, vmap), compilation profiles that use the ones there are
	std::vector<compileTool_t>		DetectTools(const std::string &dir);
	std::vector<compileProfile_t>	DefaultCompileProfiles(const std::vector<compileTool_t> &tools);

	// an engine profile for an engine next to the editor, then a game profile on
	// it for each folder next to the editor that has an FGD in it
	std::vector<gameConfig_t>	Detect();

	void	RegisterSettings();		// before the first NewFrame
}

#endif
