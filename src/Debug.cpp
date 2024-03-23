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

namespace icon7
{
void Debug(const char *file, int line, const char *function, const char *fmt,
		   ...)
{
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

	va_list va;
	va_start(va, fmt);
	static std::mutex mutex;
	std::lock_guard lock(mutex);
	fprintf(stdout, "%s:%i\t\t%s\t\t [ %2i ] \t ", file, line, funcName.c_str(),
			id);
	vfprintf(stdout, fmt, va);
	fprintf(stdout, "\n");
	fflush(stdout);
}
}
