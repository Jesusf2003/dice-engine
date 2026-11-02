//==============================
//	process.cpp
//==============================

#include "core/utils/process.h"

#include <cstring>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/*
==================
LineSplitter

bytes into lines: \n ends one, \r\n too, a lone \r starts the line over (the
tools' progress counters), so only what it settles on is kept
==================
*/
namespace
{
struct LineSplitter
{
	const std::function<void(const std::string &)> &onLine;
	std::string	line;
	bool		cr = false;

	void Feed(const char *data, size_t size)
	{
		for (size_t i = 0; i < size; i++)
		{
			char c = data[i];
			if (cr && c != '\n')
				line.clear();
			cr = false;
			if (c == '\r')
				cr = true;
			else if (c == '\n')
			{
				onLine(line);
				line.clear();
			}
			else
				line += c;
		}
	}

	void Flush()
	{
		if (!line.empty())
			onLine(line);
		line.clear();
	}
};
}

#ifdef _WIN32

static std::wstring Widen(const std::string &s)
{
	if (s.empty())
		return std::wstring();
	int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
	return w;
}

static std::string LastError()
{
	DWORD code = GetLastError();
	wchar_t *msg = nullptr;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, code, 0, (wchar_t *)&msg, 0, nullptr);
	std::string out;
	if (msg)
	{
		int n = WideCharToMultiByte(CP_UTF8, 0, msg, -1, nullptr, 0, nullptr, nullptr);
		out.resize(n > 0 ? n - 1 : 0);
		if (n > 1)
			WideCharToMultiByte(CP_UTF8, 0, msg, -1, &out[0], n, nullptr, nullptr);
		LocalFree(msg);
	}
	while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' '))
		out.pop_back();
	return out.empty() ? "error " + std::to_string(code) : out;
}

static bool Start(const std::string &program, const std::string &parameters, const std::string &workDir,
	HANDLE output, DWORD flags, PROCESS_INFORMATION &pi, std::string &error)
{
	std::wstring cmd = L"\"" + Widen(program) + L"\"";
	if (!parameters.empty())
		cmd += L" " + Widen(parameters);
	std::wstring dir = Widen(workDir);

	STARTUPINFOW si = {};
	si.cb = sizeof(si);
	if (output)
	{
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = output;
		si.hStdError = output;
	}

	if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, output != nullptr, flags, nullptr,
			dir.empty() ? nullptr : dir.c_str(), &si, &pi))
	{
		error = LastError();
		return false;
	}
	return true;
}

int Process::Run(const std::string &program, const std::string &parameters, const std::string &workDir,
	const std::function<void(const std::string &)> &onLine, const std::atomic<bool> &cancel, std::string &error)
{
	SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
	HANDLE readPipe, writePipe;
	if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
	{
		error = LastError();
		return -1;
	}
	SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

	PROCESS_INFORMATION pi = {};
	bool started = Start(program, parameters, workDir, writePipe, CREATE_NO_WINDOW, pi, error);
	CloseHandle(writePipe);		// the child has its own, the pipe ends when it exits
	if (!started)
	{
		CloseHandle(readPipe);
		return -1;
	}

	LineSplitter lines{ onLine };
	char buf[4096];
	bool killed = false;
	for (;;)
	{
		if (cancel && !killed)
		{
			TerminateProcess(pi.hProcess, 1);
			killed = true;
		}

		DWORD avail = 0;
		if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &avail, nullptr))
			break;		// closed: the process is gone
		if (!avail)
		{
			if (WaitForSingleObject(pi.hProcess, 20) == WAIT_OBJECT_0)
			{
				// what it printed last
				while (PeekNamedPipe(readPipe, nullptr, 0, nullptr, &avail, nullptr) && avail)
				{
					DWORD got = 0;
					if (!ReadFile(readPipe, buf, sizeof(buf), &got, nullptr) || !got)
						break;
					lines.Feed(buf, got);
				}
				break;
			}
			continue;
		}
		DWORD got = 0;
		if (!ReadFile(readPipe, buf, sizeof(buf), &got, nullptr) || !got)
			break;
		lines.Feed(buf, got);
	}
	lines.Flush();

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD code = 0;
	GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(readPipe);

	if (killed)
	{
		error = "terminated";
		return -1;
	}
	return (int)code;
}

bool Process::Launch(const std::string &program, const std::string &parameters, const std::string &workDir, std::string &error)
{
	PROCESS_INFORMATION pi = {};
	if (!Start(program, parameters, workDir, nullptr, DETACHED_PROCESS, pi, error))
		return false;
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return true;
}

#else	// !_WIN32

// "program" parameters through the shell, which splits them as the user quoted them
static std::string ShellCommand(const std::string &program, const std::string &parameters)
{
	std::string cmd = "'";
	for (char c : program)
		cmd += c == '\'' ? std::string("'\\''") : std::string(1, c);
	cmd += "'";
	if (!parameters.empty())
		cmd += " " + parameters;
	return cmd;
}

int Process::Run(const std::string &program, const std::string &parameters, const std::string &workDir,
	const std::function<void(const std::string &)> &onLine, const std::atomic<bool> &cancel, std::string &error)
{
	int fds[2];
	if (pipe(fds) != 0)
	{
		error = strerror(errno);
		return -1;
	}

	std::string cmd = ShellCommand(program, parameters);
	pid_t pid = fork();
	if (pid < 0)
	{
		error = strerror(errno);
		close(fds[0]);
		close(fds[1]);
		return -1;
	}
	if (pid == 0)
	{
		setpgid(0, 0);		// its own group, so a kill takes the shell's children too
		dup2(fds[1], 1);
		dup2(fds[1], 2);
		close(fds[0]);
		close(fds[1]);
		if (!workDir.empty() && chdir(workDir.c_str()) != 0)
			_exit(127);
		execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)nullptr);
		_exit(127);
	}
	close(fds[1]);

	LineSplitter lines{ onLine };
	char buf[4096];
	bool killed = false;
	for (;;)
	{
		if (cancel && !killed)
		{
			kill(-pid, SIGKILL);
			killed = true;
		}
		pollfd p = { fds[0], POLLIN, 0 };
		int r = poll(&p, 1, 20);
		if (r < 0 && errno != EINTR)
			break;
		if (r <= 0)
			continue;
		ssize_t got = read(fds[0], buf, sizeof(buf));
		if (got <= 0)
			break;
		lines.Feed(buf, (size_t)got);
	}
	lines.Flush();
	close(fds[0]);

	int status = 0;
	waitpid(pid, &status, 0);
	if (killed)
	{
		error = "terminated";
		return -1;
	}
	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) == 127)
			error = "couldn't run " + program;
		return WEXITSTATUS(status);
	}
	error = "killed by a signal";
	return -1;
}

bool Process::Launch(const std::string &program, const std::string &parameters, const std::string &workDir, std::string &error)
{
	std::string cmd = ShellCommand(program, parameters);
	pid_t pid = fork();
	if (pid < 0)
	{
		error = strerror(errno);
		return false;
	}
	if (pid == 0)
	{
		// a grandchild, so nothing waits on it
		setsid();
		if (fork() != 0)
			_exit(0);
		if (!workDir.empty() && chdir(workDir.c_str()) != 0)
			_exit(127);
		int null = open("/dev/null", O_RDWR);
		dup2(null, 0);
		dup2(null, 1);
		dup2(null, 2);
		execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)nullptr);
		_exit(127);
	}
	int status = 0;
	waitpid(pid, &status, 0);
	return true;
}

#endif
