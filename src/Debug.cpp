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

#include "../include/icon7/Time.hpp"

#include "../include/icon7/Debug.hpp"

#ifdef __unix__
# include <sys/syscall.h>
# include <unistd.h>
# ifdef SYS_gettid
static uint64_t GenThreadId()
{
	return syscall(SYS_gettid);
}
#  define GEN_THREAD_ID() GenThreadId()
# endif
#endif
#if !defined(GEN_THREAD_ID)
# include <atomic>
static uint64_t GenThreadId()
{
	static std::atomic<uint64_t> globID = 1;
	return globID++;
}
# define GEN_THREAD_ID() GenThreadId()
#endif

#ifndef ICON7_LOG_DATETIME_SUBSECONDS_DIGITS
# define ICON7_LOG_DATETIME_SUBSECONDS_DIGITS 5
#endif

namespace icon7
{
namespace log
{
#ifndef ICON7_LOG_IGNORE_COMMON_PATH
# define ICON7_LOG_IGNORE_COMMON_PATH ""
#endif
inline const static std::string IGNORE_COMMON_PATH =
	ICON7_LOG_IGNORE_COMMON_PATH;

#ifndef ICON7_LOG_DEFAULT_LOG_LEVEL
# define ICON7_LOG_DEFAULT_LOG_LEVEL icon7::log::IGNORE
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
#ifdef ICON7_LOG_LEVEL_SINGLE_CHARACTER
	static char ar[256][4];
	switch (level) {
	case FATAL:
		return "F";
	case ERROR:
		return "E";
	case WARN:
		return "W";
	case INFO:
		return "I";
	case DEBUG:
		return "D";
	case TRACE:
		return "T";
	default:
		sprintf(ar[level], "%5u", (int)level);
		return ar[level];
	}
	return "UNDEF";
#else
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
#endif
}

std::string CorrectFilePath(std::string path)
{
#ifdef ICON7_LOG_USE_FILENAME_WITHOUT_PATH
	auto it = path.rfind("\\");
	if (it == std::string::npos) {
		it = 0;
	}
	auto it2 = path.rfind("/");
	if (it2 == std::string::npos) {
		it2 = 0;
	}
	if (it < it2) {
		it = it2;
	}
	if (it != 0) {
		return path.substr(it+1);
	}
	return path;
#else
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

	auto oldSize = path.size() + 1;
	while (oldSize != path.size()) {
		oldSize = path.size();
		const auto pos = path.find("./");
		if (pos != std::string::npos) {
			if (pos >= 0) {
				if ((pos > 0 && path[pos - 1] == '/') || pos == 0) {
					path.replace(pos, 2, "");
				}
			}
		}
	}

	oldSize = path.size() + 1;
	while (oldSize != path.size()) {
		oldSize = path.size();

		const auto pos = path.find("/../");
		if (pos != std::string::npos) {
			if (pos > 0) {
				auto pos2 = path.rfind('/', pos - 1);
				if (pos2 == std::string::npos) {
					pos2 = 0;
				}
				path.replace(pos2, pos + 3 - pos2, "");
			}
		}
	}

	if (pos == 0 && path.size() > 0) {
		if (path[0] == '/' || path[0] == '\\') {
			path.erase(path.begin());
		}
	}
	return path;
#endif
}

static bool enablePrintingTime = false;

void GlobalDisablePrintingTime(bool disableTime)
{
	enablePrintingTime = !disableTime;
}

std::string GetPrettyFunctionName(const std::string function)
{
#ifdef ICON7_LOG_USE_PRETTY_FUNCTION
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
#else
	return function + "()";
#endif
}

static thread_local uint64_t threadId = GEN_THREAD_ID();
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
		timestamp = icon7::time::GetCurrentTimestampString(ICON7_LOG_DATETIME_SUBSECONDS_DIGITS);
	}

	std::string funcName = GetPrettyFunctionName(function);

	va_list va;
	va_start(va, fmt);

	constexpr int BYTES = 16 * 1024;
	char buf[BYTES];
	int offset = 0;

	snprintf(buf + offset, BYTES - offset, "%s ", LogLevelToName(logLevel));

	if (printTime) {
		offset = strlen(buf);
		snprintf(buf + offset, BYTES - offset, "%s [%3lu] ", timestamp.c_str(), threadId);
	}

	if (file != nullptr) {
		std::string filePath = file != nullptr ? CorrectFilePath(file) : "";
		offset = strlen(buf);
		snprintf(buf + offset, BYTES - offset, "%s:%i ", filePath.c_str(),
				 line);
	}
	offset = strlen(buf);
	snprintf(buf + offset, BYTES - offset, "%s : ", funcName.c_str());
	offset = strlen(buf);
	vsnprintf(buf + offset, BYTES - offset, fmt, va);
	offset = strlen(buf);
	snprintf(buf + offset, BYTES - offset, "\n");
	offset = strlen(buf);

	std::lock_guard lock(mutex);
	fwrite(buf, 1, offset, stdout);
	fflush(stdout);
}

void HexDump(void *buf, int bytes)
{
	std::lock_guard lock(mutex);
	printf("Hex dump from [%p] bytes %i\n", buf, bytes);
	for (int i = 0; i < bytes; i += 16) {
		for (int j = 0; j < 16 && j + i < bytes; ++j) {
			char c = ((char *)buf)[i + j];
			if (c >= ' ' && c < 127) {
				printf("  %c", c);
			} else {
				printf("   ");
			}
		}
		printf("\n");
		for (int j = 0; j < 16 && j + i < bytes; ++j) {
			printf(" %2.2X", (unsigned int)(((unsigned char *)buf)[i + j]));
		}
		printf("\n");
	}
	fflush(stdout);
}
} // namespace log
} // namespace icon7
