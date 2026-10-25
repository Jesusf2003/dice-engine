#pragma once
//What the core needs from whoever embeds it (the ImGui editor, a test, a command line tool).
//Replaces GtkRadiant's Sys_Printf/Sys_FPrintf and gtk_MessageBox; every callback is optional.

#include <functional>
#include <string_view>
#include <utility>

namespace radiant {

enum class LogLevel
{
	Info,
	Warning,
	Error,
};

class DocumentHost
{
public:
	using LogCallback = std::function<void(LogLevel level, std::string_view message)>;
	using ConfirmCallback = std::function<bool(std::string_view question, bool defaultAnswer)>;

	DocumentHost() = default;
	DocumentHost(LogCallback log, ConfirmCallback confirm) : m_log(std::move(log)), m_confirm(std::move(confirm)) {}

	void setLogCallback(LogCallback cb) { m_log = std::move(cb); }
	void setConfirmCallback(ConfirmCallback cb) { m_confirm = std::move(cb); }

	void log(LogLevel level, std::string_view message) const
	{
		if (m_log)
			m_log(level, message);
	}
	void info(std::string_view message) const { log(LogLevel::Info, message); }
	void warning(std::string_view message) const { log(LogLevel::Warning, message); }
	void error(std::string_view message) const { log(LogLevel::Error, message); }

	//Yes/no question for the user. Headless hosts (no callback) get defaultAnswer,
	//so pick the safe choice as the default.
	bool confirm(std::string_view question, bool defaultAnswer) const
	{
		return m_confirm ? m_confirm(question, defaultAnswer) : defaultAnswer;
	}

private:
	LogCallback m_log;
	ConfirmCallback m_confirm;
};

} // namespace radiant
