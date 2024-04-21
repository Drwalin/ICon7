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
#include <cstdio>
#include <cstring>

#include <mutex>
#include <regex>
#include <unordered_map>
#include <atomic>

#include "../include/icon7/Time.hpp"

#include "../include/icon7/Debug.hpp"

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
		return "WARN ";
	case INFO:
		return "INFO ";
	case DEBUG:
		return "DEBUG";
	case TRACE:
		return "TRACE";
	default:
		sprintf(ar[level], "%5u", (int)level);
		return ar[level];
	}
	return "UNDEF";
}

std::string CorrectFilePath(std::string path)
{
	auto pos = path.find(IGNORE_COMMON_PATH);
	if (pos == 0) {
		path.replace(pos, IGNORE_COMMON_PATH.size(), "");
	}
	while (true) {
		auto pos = path.find("\\");
		if (pos == std::string::npos) {
			break;
		}
		path.replace(pos, 1, "/");
	}
	while (true) {
		auto pos = path.find("//");
		if (pos == std::string::npos) {
			break;
		}
		path.replace(pos, 2, "/");
	}
	
	auto oldSize = path.size()+1;
	while (oldSize != path.size()) {
		oldSize = path.size();
		
		auto pos = path.find("../");
		if (pos != std::string::npos) {
			if (pos == 0 || path[pos-1] == '/') {
				auto pos2 = path.find('/', pos+3);
				if (pos2 != std::string::npos) {
					path.replace(pos, pos2-pos, "");
				}
			}
		}
	}
	
	if (pos == 0 && path.size() > 0) {
		if (path[0] == '/' || path[0] == '\\') {
			path.erase(path.begin());
		}
	}
	return path;
}

static bool enablePrintingTime = false;

void GlobalDisablePrintingTime(bool disableTime)
{
	enablePrintingTime = !disableTime;
}


std::string GetPrettyFunctionName(const std::string function) {
	thread_local std::unordered_map<std::string, std::string> names;
	auto it = names.find(function);
	if (it != names.end()) {
		return it->second;
	}
	
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
	names[function] = funcName;
	return funcName;
}


static std::atomic<int> globID = 1;
static thread_local int threadId = globID++;
static std::mutex mutex;

void Log(LogLevel logLevel, bool printTime, bool printFile, const char *file,
		 int line, const char *function, const char *fmt, ...)
{
	printTime |= enablePrintingTime;
	if (IsLogLevelApplicable(logLevel) == false) {
		return;
	}
	
	std::string timestamp;
	if (printTime) {
		timestamp = icon7::time::GetCurrentTimestampString(6);
	}

	std::string funcName = GetPrettyFunctionName(function);

	va_list va;
	va_start(va, fmt);
	
	constexpr int BYTES = 16*1024;
	char buf[BYTES];
	int offset = 0;
	
	snprintf(buf+offset, BYTES-offset, "%s ", LogLevelToName(logLevel));
	
	if (printTime) {
		offset = strlen(buf);
		snprintf(buf+offset, BYTES-offset, "%s ", timestamp.c_str());
	}
	
	if (file != nullptr) {
		std::string filePath = file!=nullptr ? CorrectFilePath(file) : "";
		offset = strlen(buf);
		snprintf(buf+offset, BYTES-offset, "%s:%i\t", filePath.c_str(), line);
	}
	offset = strlen(buf);
	snprintf(buf+offset, BYTES-offset, "%s\t [%3i] \t ", funcName.c_str(), threadId);
	offset = strlen(buf);
	vsnprintf(buf+offset, BYTES-offset, fmt, va);
	offset = strlen(buf);
	snprintf(buf+offset, BYTES-offset, "\n");
	offset = strlen(buf);
	
	{
		const uint64_t t = icon7::time::GetTimestamp();
		const std::string s = icon7::time::TimestampToString2(t, 9);
		printf("timestamp = %lu   ===   %s\n", t, s.c_str());
		const uint64_t t2 = icon7::time::StringToTimestamp(s);
		int64_t diff = t2-t;
		if (t != t2) {
			printf("DUPA, timestampy nie równe: diff = %f s         < %s\n", diff/1000000000.0, s.c_str());
		}
	}
	
	std::lock_guard lock(mutex);
	fwrite(buf, 1, offset, stdout);
	fflush(stdout);
	
	exit(0);
}
} // namespace log
} // namespace icon7
