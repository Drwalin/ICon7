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

#ifndef ICON7_DEBUG_HPP
#define ICON7_DEBUG_HPP

#define DEBUG(...)                                                             \
	icon7::log::Log(icon7::log::DEBUG, true, true, __FILE__, __LINE__,         \
					__PRETTY_FUNCTION__, __VA_ARGS__)

#define LOG_FULL(LEVEL, USE_DATE, USE_TIME, ...)                               \
	icon7::log::Log(icon7::log::LogLevel::LEVEL, USE_DATE, USE_TIME, __FILE__, \
					__LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#define LOG(LEVEL, ...) LOG_FULL(LEVEL, true, true, __VA_ARGS__)
#define LOG_(LEVEL, ...) LOG_FULL(LEVEL, false, false, __VA_ARGS__)
#define LOG_D(LEVEL, ...) LOG_FULL(LEVEL, true, false, __VA_ARGS__)
#define LOG_T(LEVEL, ...) LOG_FULL(LEVEL, false, true, __VA_ARGS__)
#define LOG_DT(LEVEL, ...) LOG_FULL(LEVEL, true, true, __VA_ARGS__)

#define LOG_FATAL(...) LOG_FULL(FATAL, true, true, __VA_ARGS__)
#define LOG_FATAL_(...) LOG_FULL(FATAL, false, false, __VA_ARGS__)
#define LOG_FATAL_D(...) LOG_FULL(FATAL, true, false, __VA_ARGS__)
#define LOG_FATAL_T(...) LOG_FULL(FATAL, false, true, __VA_ARGS__)
#define LOG_FATAL_DT(...) LOG_FULL(FATAL, true, true, __VA_ARGS__)

#define LOG_ERROR(...) LOG_FULL(ERROR, true, true, __VA_ARGS__)
#define LOG_ERROR_(...) LOG_FULL(ERROR, false, false, __VA_ARGS__)
#define LOG_ERROR_D(...) LOG_FULL(ERROR, true, false, __VA_ARGS__)
#define LOG_ERROR_T(...) LOG_FULL(ERROR, false, true, __VA_ARGS__)
#define LOG_ERROR_DT(...) LOG_FULL(ERROR, true, true, __VA_ARGS__)

#define LOG_WARN(...) LOG_FULL(WARN, true, true, __VA_ARGS__)
#define LOG_WARN_(...) LOG_FULL(WARN, false, false, __VA_ARGS__)
#define LOG_WARN_D(...) LOG_FULL(WARN, true, false, __VA_ARGS__)
#define LOG_WARN_T(...) LOG_FULL(WARN, false, true, __VA_ARGS__)
#define LOG_WARN_DT(...) LOG_FULL(WARN, true, true, __VA_ARGS__)

#define LOG_INFO(...) LOG_FULL(INFO, true, true, __VA_ARGS__)
#define LOG_INFO_(...) LOG_FULL(INFO, false, false, __VA_ARGS__)
#define LOG_INFO_D(...) LOG_FULL(INFO, true, false, __VA_ARGS__)
#define LOG_INFO_T(...) LOG_FULL(INFO, false, true, __VA_ARGS__)
#define LOG_INFO_DT(...) LOG_FULL(INFO, true, true, __VA_ARGS__)

#define LOG_DEBUG(...) LOG_FULL(DEBUG, true, true, __VA_ARGS__)
#define LOG_DEBUG_(...) LOG_FULL(DEBUG, false, false, __VA_ARGS__)
#define LOG_DEBUG_D(...) LOG_FULL(DEBUG, true, false, __VA_ARGS__)
#define LOG_DEBUG_T(...) LOG_FULL(DEBUG, false, true, __VA_ARGS__)
#define LOG_DEBUG_DT(...) LOG_FULL(DEBUG, true, true, __VA_ARGS__)

#define LOG_TRACE(...) LOG_FULL(TRACE, true, true, __VA_ARGS__)
#define LOG_TRACE_(...) LOG_FULL(TRACE, false, false, __VA_ARGS__)
#define LOG_TRACE_D(...) LOG_FULL(TRACE, true, false, __VA_ARGS__)
#define LOG_TRACE_T(...) LOG_FULL(TRACE, false, true, __VA_ARGS__)
#define LOG_TRACE_DT(...) LOG_FULL(TRACE, true, true, __VA_ARGS__)

namespace icon7
{
namespace log
{
enum LogLevel : unsigned char {
	FATAL = 1,
	ERROR = 2,
	WARN = 3,
	INFO = 4,
	DEBUG = 5,
	TRACE = 6,

	IGNORE = 127
};

void GlobalDisablePrintingDateTime(bool disableDateTime);

const char *LogLevelToName(LogLevel level);

LogLevel GetGlobalLogLevel();
void SetGlobalLogLevel(LogLevel level);

LogLevel GetThreadLocalLogLevel();
bool SetThreadLocalLogLevel(LogLevel level);
void RemoveThreadLocalLogLevel();

bool IsLogLevelApplicable(LogLevel level);

void Log(LogLevel logLevel, bool printDate, bool printTime, const char *file,
		 int line, const char *function, const char *fmt, ...);
} // namespace log
} // namespace icon7

#endif
