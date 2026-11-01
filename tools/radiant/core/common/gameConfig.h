//==============================
//	gameConfig.h
//
//	the games and mods the editor can map for, hammer's "Game Configurations"
//	(Tools > Options): each one names its game data files (FGDs) and folders.
//	the wads aren't here, they're in the FGD's wadlist, relative to these folders.
//	stored in radiant.ini as [RadiantGames][GameN] entries.
//
//	a mod is a configuration with a mod folder on top of its base game's: wads,
//	the palette (gfx/palette.lmp) and maps are looked for in the mod first.
//	paths can be relative to the editor's folder, so a configuration keeps
//	working when the whole bin folder moves
//==============================

#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

#include <string>
#include <vector>

struct gameConfig_t
{
	std::string	name;
	std::vector<std::string>	fgds;	// game data files, loaded in order: a later one redefines classes
	std::string	executableDir;			// where the engine is, for running maps (later)
	std::string	gameDir;				// the base game's folder: ninjawars, like quake's id1
	std::string	modDir;					// optional: a mod's folder, searched before gameDir
	std::string	mapsDir;				// File > Open starts here; empty: <mod or game>/maps
	std::string	defaultPointClass;		// hammer's defaults for new entities (used once entities can be placed)
	std::string	defaultSolidClass;
};

namespace GameConfigs
{
	extern std::vector<gameConfig_t>	configs;
	extern std::string					activeName;

	const gameConfig_t	*Active();		// nullptr when there's none
	void	Apply(const std::vector<gameConfig_t> &list, const std::string &active);	// marks radiant.ini for saving

	std::string	EditorDir();						// the folder the editor runs from
	std::string	Resolve(const std::string &path);	// relative paths are from EditorDir
	std::string	MakeRelative(const std::string &path);	// inside EditorDir: relative, so it travels with it

	std::vector<std::string>	SearchPaths(const gameConfig_t &cfg);	// absolute: mod, then game
	std::string	MapsDir(const gameConfig_t &cfg);

	// the folders next to the editor that have an FGD in them, one configuration each
	std::vector<gameConfig_t>	Detect();

	void	RegisterSettings();		// before the first NewFrame
}

#endif
