// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_DEBUG_HPP
#define ICON7_DEBUG_HPP

#ifdef ICON7_LOG_USE_PRETTY_FUNCTION
#define __FUNCTION_NAME__ __PRETTY_FUNCTION__
#else
#define __FUNCTION_NAME__ __FUNCTION__
#endif

#define DEBUG(...)                                                             \
	icon7::log::Log(icon7::log::DEBUG, true, true, __FILE__, __LINE__,         \
					__FUNCTION_NAME__, __VA_ARGS__)

#define LOG_FULL(LEVEL, USE_TIME, PRINT_FILE, ...)                             \
	icon7::log::Log(icon7::log::LogLevel::LEVEL, USE_TIME, PRINT_FILE,         \
					__FILE__, __LINE__, __FUNCTION_NAME__, __VA_ARGS__)

#define LOG(LEVEL, ...) LOG_FULL(LEVEL, true, true, __VA_ARGS__)
#define LOG_(LEVEL, ...) LOG_FULL(LEVEL, false, false, __VA_ARGS__)
#define LOG_T(LEVEL, ...) LOG_FULL(LEVEL, true, false, __VA_ARGS__)
#define LOG_F(LEVEL, ...) LOG_FULL(LEVEL, false, true, __VA_ARGS__)
#define LOG_TF(LEVEL, ...) LOG_FULL(LEVEL, true, true, __VA_ARGS__)

#define LOG_FATAL_ARGS(USE_TIME, USE_FILE, ...)                                \
	LOG_FULL(FATAL, USE_TIME, USE_FILE, __VA_ARGS__)
#define LOG_FATAL(...) LOG_FATAL_ARGS(true, true, __VA_ARGS__)
#define LOG_FATAL_(...) LOG_FATAL_ARGS(false, false, __VA_ARGS__)
#define LOG_FATAL_T(...) LOG_FATAL_ARGS(true, false, __VA_ARGS__)
#define LOG_FATAL_F(...) LOG_FATAL_ARGS(false, true, __VA_ARGS__)
#define LOG_FATAL_TF(...) LOG_FATAL_ARGS(true, true, __VA_ARGS__)

#define LOG_ERROR_ARGS(USE_TIME, USE_FILE, ...)                                \
	LOG_FULL(LL_ERROR, USE_TIME, USE_FILE, __VA_ARGS__)
#define LOG_ERROR(...) LOG_ERROR_ARGS(true, true, __VA_ARGS__)
#define LOG_ERROR_(...) LOG_ERROR_ARGS(false, false, __VA_ARGS__)
#define LOG_ERROR_T(...) LOG_ERROR_ARGS(true, false, __VA_ARGS__)
#define LOG_ERROR_F(...) LOG_ERROR_ARGS(false, true, __VA_ARGS__)
#define LOG_ERROR_TF(...) LOG_ERROR_ARGS(true, true, __VA_ARGS__)

#ifndef ICON7_DISABLE_LOG_LEVEL_WARN
#define LOG_WARN_ARGS(USE_TIME, USE_FILE, ...)                                 \
	LOG_FULL(WARN, USE_TIME, USE_FILE, __VA_ARGS__)
#else
#define ICON7_DISABLE_LOG_LEVEL_INFO
#define LOG_WARN_ARGS(USE_TIME, USE_FILE, ...)
#endif
#define LOG_WARN(...) LOG_WARN_ARGS(true, true, __VA_ARGS__)
#define LOG_WARN_(...) LOG_WARN_ARGS(false, false, __VA_ARGS__)
#define LOG_WARN_T(...) LOG_WARN_ARGS(true, false, __VA_ARGS__)
#define LOG_WARN_F(...) LOG_WARN_ARGS(false, true, __VA_ARGS__)
#define LOG_WARN_TF(...) LOG_WARN_ARGS(true, true, __VA_ARGS__)

#ifndef ICON7_DISABLE_LOG_LEVEL_INFO
#define LOG_INFO_ARGS(USE_TIME, USE_FILE, ...)                                 \
	LOG_FULL(INFO, USE_TIME, USE_FILE, __VA_ARGS__)
#else
#define ICON7_DISABLE_LOG_LEVEL_DEBUG
#define LOG_INFO_ARGS(USE_TIME, USE_FILE, ...)
#endif
#define LOG_INFO(...) LOG_INFO_ARGS(true, true, __VA_ARGS__)
#define LOG_INFO_(...) LOG_INFO_ARGS(false, false, __VA_ARGS__)
#define LOG_INFO_T(...) LOG_INFO_ARGS(true, false, __VA_ARGS__)
#define LOG_INFO_F(...) LOG_INFO_ARGS(false, true, __VA_ARGS__)
#define LOG_INFO_TF(...) LOG_INFO_ARGS(true, true, __VA_ARGS__)

#ifndef ICON7_DISABLE_LOG_LEVEL_DEBUG
#define LOG_DEBUG_ARGS(USE_TIME, USE_FILE, ...)                                \
	LOG_FULL(DEBUG, USE_TIME, USE_FILE, __VA_ARGS__)
#else
#define ICON7_DISABLE_LOG_LEVEL_TRACE
#define LOG_DEBUG_ARGS(USE_TIME, USE_FILE, ...)
#endif
#define LOG_DEBUG(...) LOG_DEBUG_ARGS(true, true, __VA_ARGS__)
#define LOG_DEBUG_(...) LOG_DEBUG_ARGS(false, false, __VA_ARGS__)
#define LOG_DEBUG_T(...) LOG_DEBUG_ARGS(true, false, __VA_ARGS__)
#define LOG_DEBUG_F(...) LOG_DEBUG_ARGS(false, true, __VA_ARGS__)
#define LOG_DEBUG_TF(...) LOG_DEBUG_ARGS(true, true, __VA_ARGS__)

#ifndef ICON7_DISABLE_LOG_LEVEL_TRACE
#define LOG_TRACE_ARGS(USE_TIME, USE_FILE, ...)                                \
	LOG_FULL(TRACE, USE_TIME, USE_FILE, __VA_ARGS__)
#else
#define LOG_TRACE_ARGS(USE_TIME, USE_FILE, ...)
#endif
#define LOG_TRACE(...) LOG_TRACE_ARGS(true, true, __VA_ARGS__)
#define LOG_TRACE_(...) LOG_TRACE_ARGS(false, false, __VA_ARGS__)
#define LOG_TRACE_T(...) LOG_TRACE_ARGS(true, false, __VA_ARGS__)
#define LOG_TRACE_F(...) LOG_TRACE_ARGS(false, true, __VA_ARGS__)
#define LOG_TRACE_TF(...) LOG_TRACE_ARGS(true, true, __VA_ARGS__)

namespace icon7
{
namespace log
{
enum LogLevel : unsigned char {
	FATAL = 1,
	LL_ERROR = 2,
	WARN = 3,
	INFO = 4,
	DEBUG = 5,
	TRACE = 6,

	LL_IGNORE = 127
};

void GlobalDisablePrintinTime(bool disableTime);

const char *LogLevelToName(LogLevel level);

LogLevel GetGlobalLogLevel();
void SetGlobalLogLevel(LogLevel level);

LogLevel GetThreadLocalLogLevel();
bool SetThreadLocalLogLevel(LogLevel level);
void RemoveThreadLocalLogLevel();

bool IsLogLevelApplicable(LogLevel level);

void Log(LogLevel logLevel, bool printTime, bool printFile, const char *file,
		 int line, const char *function, const char *fmt, ...);

void HexDump(void *buf, int bytes);
} // namespace log
} // namespace icon7

#endif
