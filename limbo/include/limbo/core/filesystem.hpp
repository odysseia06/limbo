#pragma once
#include <filesystem>
#include <string>

namespace limbo::fs {

	/// Full path of the current executable (symlinks resolved when possible).
	std::filesystem::path executable_path();

	/// Directory where log files live  (…/logs/, created on first call).
	std::filesystem::path logs_directory();

} // namespace limbo::fs

