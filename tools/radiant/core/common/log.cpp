//==============================
//	log.cpp
//==============================

#include "core/common/log.h"
#include "core/common/preferences.h"

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <mutex>

static std::deque<Log::line_t>	s_lines;
static uint64_t					s_generation;
static bool						s_errorFlag;
static std::recursive_mutex		s_mutex;		// SDL may log from its own threads

static FILE		*s_file;
static std::string	s_fileName;

static SDL_LogOutputFunction	s_prevOutput;
static void						*s_prevUserdata;

static const char *s_levelNames[LOG_LEVELS] = { "error", "warning", "info", "verbose", "debug" };

const char *Log::LevelName(LogLevel level)
{
	return (level >= 0 && level < LOG_LEVELS) ? s_levelNames[level] : "?";
}

std::string Log::FormatTime(int64_t timeMs)
{
	time_t secs = (time_t)(timeMs / 1000);
	struct tm local;
#ifdef _WIN32
	localtime_s(&local, &secs);
#else
	localtime_r(&secs, &local);
#endif
	char buf[16];
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", local.tm_hour, local.tm_min, local.tm_sec, (int)(timeMs % 1000));
	return buf;
}

static void WriteFileLine(const Log::line_t &l)
{
	fprintf(s_file, "[%s] %s: %s\n", Log::FormatTime(l.timeMs).c_str(), Log::LevelName(l.level), l.text.c_str());
}

/*
==================
OpenLogFile / CloseLogFile

opening writes out what's already in the scrollback, so the messages from
before radiant.ini was read (textures, GL) make it into the file too
==================
*/
static void CloseLogFile()
{
	if (!s_file)
		return;
	fclose(s_file);
	s_file = nullptr;
	s_fileName.clear();
}

static void OpenLogFile()
{
	const consoleAdvancedPrefs_t &adv = g_prefs.consoleAdvanced;

	if (s_file && s_fileName == adv.logFile)
		return;
	CloseLogFile();
	if (adv.logFile.empty())
		return;

#ifdef _WIN32
	if (fopen_s(&s_file, adv.logFile.c_str(), adv.appendLogFile ? "a" : "w") != 0)
		s_file = nullptr;
#else
	s_file = fopen(adv.logFile.c_str(), adv.appendLogFile ? "a" : "w");
#endif
	if (!s_file)
	{
		Log::Warning("Couldn't open the log file %s", adv.logFile.c_str());
		return;
	}
	s_fileName = adv.logFile;

	for (const Log::line_t &l : s_lines)
		WriteFileLine(l);
	fflush(s_file);
}

/*
==================
SDLLogOutput

SDL_Log goes on to wherever it went before (the debugger, stderr) and,
unless Console > Advanced says otherwise, into the console as well
==================
*/
static void SDLCALL SDLLogOutput(void *, int, SDL_LogPriority priority, const char *message)
{
	if (s_prevOutput)
		s_prevOutput(s_prevUserdata, SDL_LOG_CATEGORY_APPLICATION, priority, message);

	if (!g_prefs.consoleAdvanced.captureSDL)
		return;

	LogLevel level;
	switch (priority)
	{
	case SDL_LOG_PRIORITY_CRITICAL:
	case SDL_LOG_PRIORITY_ERROR:	level = LOG_ERROR; break;
	case SDL_LOG_PRIORITY_WARN:		level = LOG_WARNING; break;
	case SDL_LOG_PRIORITY_VERBOSE:	level = LOG_VERBOSE; break;
	case SDL_LOG_PRIORITY_DEBUG:	level = LOG_DEBUG; break;
	default:						level = LOG_INFO; break;
	}
	Log::Print(level, "%s", message);
}

void Log::Init()
{
	SDL_LogGetOutputFunction(&s_prevOutput, &s_prevUserdata);
	SDL_LogSetOutputFunction(SDLLogOutput, nullptr);
}

void Log::Shutdown()
{
	SDL_LogSetOutputFunction(s_prevOutput, s_prevUserdata);
	std::lock_guard<std::recursive_mutex> lock(s_mutex);
	CloseLogFile();
}

void Log::PrefsChanged()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	while (s_lines.size() > (size_t)g_prefs.console.scrollback)
		s_lines.pop_front();
	s_generation++;

	if (g_prefs.consoleAdvanced.logToFile)
		OpenLogFile();
	else
		CloseLogFile();
}

/*
==================
Log::Enabled

errors, warnings and info always; the detail levels only as far as
Console > Advanced asks for
==================
*/
bool Log::Enabled(LogLevel level)
{
	switch (level)
	{
	case LOG_VERBOSE:	return g_prefs.consoleAdvanced.logDetail >= MAPLOG_ENTITIES;
	case LOG_DEBUG:		return g_prefs.consoleAdvanced.logDetail >= MAPLOG_BRUSHES;
	default:			return true;
	}
}

static void PrintV(LogLevel level, const char *fmt, va_list args)
{
	if (!Log::Enabled(level))
		return;

	char stackBuf[1024];
	std::string heapBuf;
	const char *text = stackBuf;

	va_list copy;
	va_copy(copy, args);
	int len = vsnprintf(stackBuf, sizeof(stackBuf), fmt, copy);
	va_end(copy);
	if (len < 0)
		return;
	if ((size_t)len >= sizeof(stackBuf))
	{
		heapBuf.resize((size_t)len + 1);
		vsnprintf(&heapBuf[0], heapBuf.size(), fmt, args);
		heapBuf.resize((size_t)len);
		text = heapBuf.c_str();
	}

	int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	std::lock_guard<std::recursive_mutex> lock(s_mutex);

	// one entry per line, trailing newlines dropped
	const char *start = text;
	for (;;)
	{
		const char *nl = strchr(start, '\n');
		size_t n = nl ? (size_t)(nl - start) : strlen(start);

		s_lines.push_back({ level, now, std::string(start, n) });
		if (s_file)
			WriteFileLine(s_lines.back());

		if (!nl || !nl[1])
			break;
		start = nl + 1;
	}

	while (s_lines.size() > (size_t)std::max(g_prefs.console.scrollback, 100))
		s_lines.pop_front();
	if (s_file)
		fflush(s_file);

	if (level == LOG_ERROR)
		s_errorFlag = true;
	s_generation++;
}

void Log::Print(LogLevel level, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	PrintV(level, fmt, args);
	va_end(args);
}

#define LOG_LEVEL_FN(fn, level)				\
	void Log::fn(const char *fmt, ...)		\
	{										\
		va_list args;						\
		va_start(args, fmt);				\
		PrintV(level, fmt, args);			\
		va_end(args);						\
	}

LOG_LEVEL_FN(Error, LOG_ERROR)
LOG_LEVEL_FN(Warning, LOG_WARNING)
LOG_LEVEL_FN(Info, LOG_INFO)
LOG_LEVEL_FN(Verbose, LOG_VERBOSE)
LOG_LEVEL_FN(Debug, LOG_DEBUG)

size_t Log::Count()
{
	return s_lines.size();
}

const Log::line_t &Log::Line(size_t i)
{
	return s_lines[i];
}

uint64_t Log::Generation()
{
	return s_generation;
}

void Log::Clear()
{
	std::lock_guard<std::recursive_mutex> lock(s_mutex);
	s_lines.clear();
	s_generation++;
}

bool Log::TakeErrorFlag()
{
	bool f = s_errorFlag;
	s_errorFlag = false;
	return f;
}
