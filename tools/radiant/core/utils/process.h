//==============================
//	process.h
//
//	running the compilation tools and the engine: trenchbroom does it with
//	QProcess (CompilationRunToolTaskRunner, LaunchGameEngine), here it's
//	CreateProcess on windows and fork / exec elsewhere. the parameters are one
//	string, quoted the way the system's command line is.
//	paths and text are utf-8
//==============================

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <atomic>
#include <functional>
#include <string>

namespace Process
{
	/*
		runs program with parameters in workDir and waits for it, handing every
		line it prints (stdout and stderr) to onLine. a line a tool rewrites with
		a carriage return (a progress counter) comes as it is when the next one
		starts. cancel set kills it.
		the exit code; -1 and error when it couldn't start or was killed
	*/
	int		Run(const std::string &program, const std::string &parameters, const std::string &workDir,
				const std::function<void(const std::string &)> &onLine, const std::atomic<bool> &cancel, std::string &error);

	// starts program and leaves it running, trenchbroom's startDetached
	bool	Launch(const std::string &program, const std::string &parameters, const std::string &workDir, std::string &error);
}

#endif
