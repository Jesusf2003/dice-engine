//==============================
//	log.h
//
//	the editor's log: every message the console shows. quaked3's Log::Print
//	with levels, so the console can filter and color them and the map loader
//	can go from a summary down to every face (preferences, Console > Advanced).
//	SDL_Log messages are captured too, the texture and GL code still use it.
//==============================

#ifndef __LOG_H__
#define __LOG_H__

#include <cstdint>
#include <string>

enum LogLevel
{
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_VERBOSE,	// per entity detail (Console > Advanced: Verbose)
	LOG_DEBUG,		// per brush and face detail (Console > Advanced: Debug)
	LOG_LEVELS
};

namespace Log
{
	struct line_t
	{
		LogLevel	level;
		int64_t		timeMs;		// wall clock, ms since the epoch
		std::string	text;
	};

	void	Init();					// hooks SDL_Log, call once SDL is up
	void	Shutdown();
	void	PrefsChanged();			// scrollback, log file, SDL capture

	bool	Enabled(LogLevel level);	// whether Print would keep it, to skip building detail text
	void	Print(LogLevel level, const char *fmt, ...);
	void	Error(const char *fmt, ...);
	void	Warning(const char *fmt, ...);
	void	Info(const char *fmt, ...);
	void	Verbose(const char *fmt, ...);
	void	Debug(const char *fmt, ...);

	// lines are kept in a ring the size of the scrollback; Line(0) is the oldest
	size_t			Count();
	const line_t	&Line(size_t i);
	uint64_t		Generation();	// changes whenever a line is added or the log is cleared
	void			Clear();
	bool			TakeErrorFlag();	// an error was printed since the last call

	std::string		FormatTime(int64_t timeMs);		// "HH:MM:SS.mmm", local time
	const char		*LevelName(LogLevel level);
}

#endif
