//==============================
//	mapReport.cpp
//==============================

#include "map/mapReport.h"
#include "core/common/log.h"

#include <cstdarg>
#include <cstdio>

MapReport::MapReport(const std::string &file, int warningLimit) :
	fileName(file), limit(warningLimit)
{
}

void MapReport::Add(bool error, const char *kind, int line, const char *text)
{
	int count = ++perKind[kind];
	if (error)
		errors++;
	else
		warnings++;

	// errors always show, warnings only up to the limit per kind
	if (!error && limit > 0 && count > limit)
		return;

	if (line > 0)
		Log::Print(error ? LOG_ERROR : LOG_WARNING, "%s:%d: %s", fileName.c_str(), line, text);
	else
		Log::Print(error ? LOG_ERROR : LOG_WARNING, "%s: %s", fileName.c_str(), text);
}

#define REPORT_FORMAT(error)						\
	char text[1024];								\
	va_list args;									\
	va_start(args, fmt);							\
	vsnprintf(text, sizeof(text), fmt, args);		\
	va_end(args);									\
	Add(error, kind, line, text);

void MapReport::Error(const char *kind, int line, const char *fmt, ...)
{
	REPORT_FORMAT(true)
}

void MapReport::Warning(const char *kind, int line, const char *fmt, ...)
{
	REPORT_FORMAT(false)
}

void MapReport::Finish()
{
	if (limit <= 0)
		return;

	for (const auto &k : perKind)
	{
		if (k.second > limit)
			Log::Warning("%s: %d more '%s' warnings not shown (Preferences > Console > Advanced: warning limit %d)",
				fileName.c_str(), k.second - limit, k.first.c_str(), limit);
	}
}
