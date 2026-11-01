//==============================
//	mapReport.h
//
//	the problems found while loading a map, straight into the console. each
//	message has a kind ("absolute wad path", "missing texture"...): past the
//	warning limit (Console > Advanced) a kind goes quiet and Finish says how
//	many more there were, so one broken thing repeated 500 times doesn't bury
//	everything else
//==============================

#ifndef __MAP_REPORT_H__
#define __MAP_REPORT_H__

#include <map>
#include <string>

class MapReport
{
public:
	MapReport(const std::string &fileName, int warningLimit);

	// "<file>:<line>: <message>", line 0 leaves it out
	void	Error(const char *kind, int line, const char *fmt, ...);
	void	Warning(const char *kind, int line, const char *fmt, ...);
	void	Finish();		// the suppressed counts

	inline int Errors() const	{ return errors; }
	inline int Warnings() const	{ return warnings; }

private:
	void	Add(bool error, const char *kind, int line, const char *text);

	std::string	fileName;
	int		limit;
	int		errors = 0, warnings = 0;
	std::map<std::string, int>	perKind;
};

#endif
