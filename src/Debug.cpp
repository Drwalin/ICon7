// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "../include/icon7/Time.hpp"

#include "../include/icon7/Debug.hpp"

#ifdef __unix__
#include <sys/syscall.h>
#include <unistd.h>
#ifdef SYS_gettid
static uint64_t GenThreadId() { return syscall(SYS_gettid); }
static uint64_t GetProcessId() { return getpid(); }
#define GEN_THREAD_ID() GenThreadId()
#endif
#else
#include <windows.h>
static uint64_t GenThreadId() { return GetCurrentThreadId(); }
static uint64_t GetProcessId() { return GetCurrentProcessId(); }
#define GEN_THREAD_ID() GenThreadId()
#endif
#if !defined(GEN_THREAD_ID)
#include <windows.h>
#include <atomic>
static uint64_t GenThreadId()
{
	return GetCurrentThreadId();
	static std::atomic<uint64_t> globID = 1;
	return globID++;
}
static uint64_t GetProcessId() { return 0; }
#warn UNKNWON ENVIRNMENT: not *nix nor windows
#define GEN_THREAD_ID() GenThreadId()
#endif

#ifndef ICON7_LOG_USE_PRETTY_FUNCTION
#define ICON7_LOG_USE_PRETTY_FUNCTION 1
#endif

#ifndef ICON7_LOG_DATETIME_SUBSECONDS_DIGITS
#define ICON7_LOG_DATETIME_SUBSECONDS_DIGITS 5
#endif

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
#define ICON7_LOG_DEFAULT_LOG_LEVEL icon7::log::LL_IGNORE
#endif

static LogLevel globalLogLevel = ICON7_LOG_DEFAULT_LOG_LEVEL;
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
static thread_local LogLevel threadLocalLogLevel = LL_IGNORE;
#else
#endif

LogLevel GetGlobalLogLevel() { return globalLogLevel; }

void SetGlobalLogLevel(LogLevel level) { globalLogLevel = level; }

LogLevel GetThreadLocalLogLevel()
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	return threadLocalLogLevel;
#else
	return LL_IGNORE;
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
	threadLocalLogLevel = LL_IGNORE;
#else
#endif
}

bool IsLogLevelApplicable(LogLevel level)
{
#ifndef ICON7_LOG_IGNORE_THREAD_LOCAL_LOG_LEVEL
	LogLevel tlsl = GetThreadLocalLogLevel();
	if (tlsl != LL_IGNORE) {
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
	case LL_ERROR:
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
	case LL_ERROR:
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
		return path.substr(it + 1);
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
	static std::shared_mutex mutex;
	static std::unordered_map<std::string, std::string> names;
	{
		std::shared_lock lock(mutex);
		auto it = names.find(function);
		if (it != names.end()) {
			return it->second;
		}
	}

	std::string funcName = function;

	for (int i = 0; i < funcName.size(); ++i) {
		if (funcName[i] == '[') {
			for (int j = i + 1; j < funcName.size(); ++j) {
				if (funcName[j] == '[') {
					break;
				}
				if (funcName[j] == ']') {
					funcName.erase(i, j + 1 - i);
					i = 0;
					break;
				}
			}
		}
	}

	for (;;) {
		auto p = funcName.find("static");

		if (p == std::string::npos) {
			p = funcName.find("virtual");
			if (p == std::string::npos) {
				p = funcName.find("const");
				if (p == std::string::npos) {
					break;
				} else {
					funcName.erase(p, 5);
				}
			} else {
				funcName.erase(p, 7);
			}
		} else {
			funcName.erase(p, 6);
		}

		if (p < funcName.size()) {
			if (funcName[p] == ' ') {
				funcName.erase(p, 1);
			}
		}

		if (p - 1 < funcName.size() && p - 1 > 0) {
			if (funcName[p - 1] == ' ') {
				funcName.erase(p - 1, 1);
			}
		}
	}

	for (int i = 0; i < funcName.size(); ++i) {
		if (funcName[i] == '(') {
			for (int j = i + 1; j < funcName.size(); ++j) {
				if (funcName[j] == '(') {
					break;
				}
				if (funcName[j] == ')') {
					if (j == i + 1) {
						break;
					}
					funcName.replace(i, j - i + 1, "()");
					i = 0;
					break;
				}
			}
		}
	}

	while (funcName.ends_with(" ")) {
		funcName.erase(funcName.size() - 1, 1);
	}

	if (funcName.ends_with("()") == false) {
		funcName += "()";
	}

	for (auto p = funcName.find("()()"); p != std::string::npos;
		 p = funcName.find("()()")) {
		funcName.erase(p, 2);
	}

	for (auto p = funcName.find("::()::"); p != std::string::npos;
		 p = funcName.find("::()::")) {
		funcName.erase(p, 4);
	}

	{
		std::lock_guard lock(mutex);
		names[function] = funcName;
	}

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
		timestamp = icon7::time::GetCurrentTimestampString(
			ICON7_LOG_DATETIME_SUBSECONDS_DIGITS);
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
		snprintf(buf + offset, BYTES - offset, "%s [%3lu] ", timestamp.c_str(),
				 threadId);
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

	WriteSync(buf, offset);

	va_end(va);
}

static FILE *GetFile()
{
#ifdef ICON7_LOG_ONLY_ON_STDERR
	return stderr;
#else
	static FILE *file = nullptr;
	static time::Timestamp openTime = {0};
	time::Timestamp now = time::GetTimestamp();
	if (file == nullptr || openTime + icon7::time::seconds(3600 * 24) < now) {
		if (file) {
			fflush(file);
			if (file != stdout && file != stderr) {
				fclose(file);
			}
			file = nullptr;
		}
		constexpr uint32_t BYTES = 128;
		char fileName[BYTES] = {0};
		openTime = now;
		std::string timestampStr = icon7::time::TimestampToString(now, 0);
		int written = snprintf(fileName, BYTES, "%s_%lu.log",
							   timestampStr.c_str(), GetProcessId());
		if (written > 64 || written <= 0) {
			printf("DUPA1 = %i\n", written);
			file = nullptr;
		} else {
			file = fopen(fileName, "w");
		}
		if (file == nullptr) {
			printf("DUPA2\n");
			file = stderr;
		}
	}
	return file;
#endif
}

#define ICON7_MULTI_FILE_PRINT(func, ...) { \
	if (output != stderr && output != stdout) { \
		func(__VA_ARGS__); \
	} \
	FILE *tmp = output; \
	output = stderr; \
	func(__VA_ARGS__); \
	output = tmp; \
}

void HexDump(void *buf, int bytes)
{
	std::lock_guard lock(mutex);
	FILE *output = GetFile();
	ICON7_MULTI_FILE_PRINT(fprintf, output, "Hex dump from [%p] bytes %i\n", buf, bytes);
	for (int i = 0; i < bytes; i += 16) {
		for (int j = 0; j < 16 && j + i < bytes; ++j) {
			char c = ((char *)buf)[i + j];
			if (c >= ' ' && c < 127) {
				ICON7_MULTI_FILE_PRINT(fprintf, output, "  %c", c);
			} else {
				ICON7_MULTI_FILE_PRINT(fprintf, output, "   ");
			}
		}
		ICON7_MULTI_FILE_PRINT(fprintf, output, "\n");
		for (int j = 0; j < 16 && j + i < bytes; ++j) {
			ICON7_MULTI_FILE_PRINT(fprintf, output, " %2.2X",
					(unsigned int)(((unsigned char *)buf)[i + j]));
		}
		ICON7_MULTI_FILE_PRINT(fprintf, output, "\n");
	}
	ICON7_MULTI_FILE_PRINT(fflush, output);
}

void PrintLineSync(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	{
		std::lock_guard lock(mutex);
		FILE *output = GetFile();
		ICON7_MULTI_FILE_PRINT(vfprintf, output, fmt, va);
		ICON7_MULTI_FILE_PRINT(fwrite, "\n", 1, 1, output);
		ICON7_MULTI_FILE_PRINT(fflush, output);
	}
	va_end(va);
}

void WriteSync(const char *buf, int bytes)
{
	std::lock_guard lock(mutex);
	FILE *output = GetFile();
	ICON7_MULTI_FILE_PRINT(fwrite, buf, 1, bytes, output);
	ICON7_MULTI_FILE_PRINT(fflush, output);
}
#undef ICON7_MULTI_FILE_PRINT
} // namespace log
} // namespace icon7
