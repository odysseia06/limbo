#pragma once
#include <format>
#include <string_view>

namespace limbo::log {
	enum class Level {
		Trace,
		Debug,
		Info,
		Warning,
		Error,
		Fatal
	};

	void init(Level consoleMin, Level fileMin);
	void shutdown();
	void message(Level level, std::string_view fmt, std::format_args args);

	template <typename... Ts>
	inline void trace(std::string_view fmt, Ts&&... args) {
		message(Level::Trace, fmt, std::make_format_args(args...));
	}
	template <typename... Ts>
	inline void debug(std::string_view fmt, Ts&&... args) {
		message(Level::Debug, fmt, std::make_format_args(args...));
	}
	template <typename... Ts>
	inline void info(std::string_view fmt, Ts&&... args) {
		message(Level::Info, fmt, std::make_format_args(args...));
	}
	template <typename... Ts>
	inline void warning(std::string_view fmt, Ts&&... args) {
		message(Level::Warning, fmt, std::make_format_args(args...));
	}
	template <typename... Ts>
	inline void error(std::string_view fmt, Ts&&... args) {
		message(Level::Error, fmt, std::make_format_args(args...));
	}
	template <typename... Ts>
	inline void fatal(std::string_view fmt, Ts&&... args) {
		message(Level::Fatal, fmt, std::make_format_args(args...));
	}
} // namespace limbo::log
