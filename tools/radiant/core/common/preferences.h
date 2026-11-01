//==============================
//	preferences.h
//
//	editor preferences, one struct per page of the preferences window
//	(ui/preferences/wndPreferences.h). stored in radiant.ini as
//	[RadiantPrefs][<page>] entries, through ImGui's settings handlers like
//	the layout and the texture favourites
//==============================

#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <string>

// Console
struct consolePrefs_t
{
	int		scrollback = 5000;			// lines kept, older ones drop off
	bool	timestamps = false;
	bool	autoScroll = true;			// follow new lines while at the bottom
	bool	showOnError = true;			// bring the console tab forward when an error is logged
};

// Console > Advanced
enum mapLogDetail_t
{
	MAPLOG_SUMMARY,		// the load report and problems
	MAPLOG_ENTITIES,	// + a line per entity (LOG_VERBOSE)
	MAPLOG_BRUSHES,		// + every brush and face (LOG_DEBUG)
	MAPLOG_COUNT
};

struct consoleAdvancedPrefs_t
{
	int		logDetail = MAPLOG_SUMMARY;	// mapLogDetail_t, also what LOG_VERBOSE / LOG_DEBUG lines get kept
	int		warningLimit = 20;			// same kind of warning per operation before the rest are summed up, 0 = all
	bool	timings = true;				// how long parsing and validation took
	bool	captureSDL = true;			// SDL_Log messages (textures, GL) into the console
	bool	logToFile = false;
	std::string	logFile = "radiant.log";	// relative to the working directory
	bool	appendLogFile = false;		// otherwise it starts over every session

	// map load analysis
	bool	validateGeometry = true;	// build every brush from its planes and report bad ones
	bool	validateTextures = true;	// check face textures against the loaded wads
	bool	validateTargets = true;		// target / killtarget pointing at nothing
};

struct preferences_t
{
	consolePrefs_t			console;
	consoleAdvancedPrefs_t	consoleAdvanced;
};

extern preferences_t g_prefs;

namespace Preferences
{
	void	RegisterSettings();		// before the first NewFrame, that's when radiant.ini is read
	void	Apply(const preferences_t &prefs);	// takes effect now and marks radiant.ini for saving
}

#endif
