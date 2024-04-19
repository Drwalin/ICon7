/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
 *
 *  ICon7 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdarg>

#include <mutex>
#include <regex>
#include <atomic>

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Time.hpp"

namespace icon7
{
namespace log
{
#ifndef ICON7_LOG_IGNORE_COMMON_PATH
#define ICON7_LOG_IGNORE_COMMON_PATH ""
#endif
inline const static std::string IGNORE_COMMON_PATH =
	ICON7_LOG_IGNORE_COMMON_PATH;

#ifndef ICON7_LOG_DEFAULT_LOG_LEVEL
#define ICON7_LOG_DEFAULT_LOG_LEVEL icon7::log::IGNORE
#endif

static LogLevel globalLogLevel = ICON7_LOG_DEFAULT_LOG_LEVEL;
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
static thread_local LogLevel threadLocalLogLevel = IGNORE;
#else
#endif

LogLevel GetGlobalLogLevel() { return globalLogLevel; }

void SetGlobalLogLevel(LogLevel level) { globalLogLevel = level; }

LogLevel GetThreadLocalLogLevel()
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	return threadLocalLogLevel;
#else
	return IGNORE;
#endif
}

bool SetThreadLocalLogLevel(LogLevel level)
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	threadLocalLogLevel = level;
	return true;
#else
	return false;
#endif
}

void RemoveThreadLocalLogLevel()
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	threadLocalLogLevel = IGNORE;
#else
#endif
}

bool IsLogLevelApplicable(LogLevel level)
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	LogLevel tlsl = GetThreadLocalLogLevel();
	if (tlsl != IGNORE) {
		return level <= tlsl;
	}
#endif
	return level <= globalLogLevel;
}

const char *LogLevelToName(LogLevel level)
{
	static char ar[256][4];
	switch (level) {
	case FATAL:
		return "FATAL";
	case ERROR:
		return "ERROR";
	case WARN:
		return "WARN";
	case INFO:
		return "INFO";
	case DEBUG:
		return "DEBUG";
	case TRACE:
		return "TRACE";
	default:
		sprintf(ar[level], "%u", (int)level);
		return ar[level];
	}
	return "UNDEF";
}

std::string CorrectFilePath(std::string path)
{
	if (path.size() > 1) {
		auto pos = path.find(IGNORE_COMMON_PATH);
		if (pos == 0) {
			path.replace(pos, IGNORE_COMMON_PATH.size(), "");
		}
	}
	const static std::regex findParentPathRegex(
		"\\.\\./[^/\\\\]*[^./\\\\][^/\\\\]/");
	const static std::regex findThisPathRegex("/\\./");
	for (int i = 0; i < 100; ++i) {
		auto oldSize = path.size();
		path = std::regex_replace(path, findParentPathRegex, "");
		path = std::regex_replace(path, findThisPathRegex, "/");
		if (oldSize == path.size()) {
			break;
		}
	}
	return path;
}

static bool enablePrintingDateTime = false;

void GlobalDisablePrintingDateTime(bool disableDateTime)
{
	enablePrintingDateTime = !disableDateTime;
}

void Log(LogLevel logLevel, bool printDate, bool printTime, const char *file,
		 int line, const char *function, const char *fmt, ...)
{
	printDate |= enablePrintingDateTime;
	printTime |= enablePrintingDateTime;
	if (IsLogLevelApplicable(logLevel) == false) {
		return;
	}

	static std::atomic<int> globID = 1;
	thread_local static int id = globID++;

	std::string funcName = function;
	funcName = std::regex_replace(funcName, std::regex("\\[[^\\[\\]]+\\]"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	funcName =
		std::regex_replace(funcName, std::regex("\\([^\\(\\)]+\\)"), "()");
	funcName = std::regex_replace(
		funcName, std::regex(".*[: >]([a-zA-Z0-9_]*::[a-z0-9A-Z_]*)+\\(.*"),
		"$1");
	funcName += "()";

	std::string filePath = CorrectFilePath(file);

	std::string date, time;
	if (printDate || printTime) {
		icon7::time::GetCurrentDateTimeStrings(date, time, 5);
	}
	if (!printDate) {
		date = "";
	}
	if (!printTime) {
		time = "";
	}

	va_list va;
	va_start(va, fmt);
	static std::mutex mutex;
	std::lock_guard lock(mutex);
	fprintf(stdout, "[%s]%s%s%s%s: %s:%i\t%s\t [ %2i ] \t ",
			LogLevelToName(logLevel),
			(printDate?" ":""), date.c_str(),
			(printTime?" ":""), time.c_str(),
			filePath.c_str(), line, funcName.c_str(), id);
	vfprintf(stdout, fmt, va);
	fprintf(stdout, "\n");
	fflush(stdout);
}
} // namespace log
} // namespace icon7
