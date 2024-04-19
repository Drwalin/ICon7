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
	icon7::log::Log(icon7::log::DEBUG, false, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

namespace icon7
{
namespace log
{
enum LogLevel : unsigned char
{
	FATAL = 1,
	ERROR = 2,
	WARN = 3,
	INFO = 4,
	DEBUG = 5,
	TRACE = 6,
	
	IGNORE = 127
};

const char *LogLevelToName(LogLevel level);

LogLevel GetGlobalLogLevel();
void SetGlobalLogLevel(LogLevel level);

LogLevel GetThreadLocalLogLevel();
bool SetThreadLocalLogLevel(LogLevel level);
void RemoveThreadLocalLogLevel();

bool IsLogLevelApplicable(LogLevel level);

void Log(LogLevel logLevel, bool printDate, bool printTime, const char *file, int line,
		const char *function, const char *fmt, ...);
}
}

#endif
