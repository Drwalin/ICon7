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

#include <cstring>

#include <chrono>

#include "../include/icon7/Time.hpp"

namespace icon7
{
namespace time
{
struct DayTimeBegin {
	DayTimeBegin()
		: date(std::chrono::system_clock::now()),
		  time(std::chrono::steady_clock::now())
	{
	}
	const std::chrono::system_clock::time_point date;
	const std::chrono::steady_clock::time_point time;
};
static DayTimeBegin timeOriginHolder;
void GetCurrentDateTimeStrings(std::string &sdate, std::string &stime,
						int subsecondDigits)
{
	static DayTimeBegin glob;
	
	auto now = std::chrono::steady_clock::now();

	auto diff = now - glob.time;

	auto secondsDuration = std::chrono::floor<std::chrono::seconds>(diff);

	auto _date = glob.date + secondsDuration;

	auto subseconds =
		std::chrono::duration_cast<std::chrono::nanoseconds, int64_t>(
			diff - secondsDuration);
	char subsecondsStr[32];
	auto ns = subseconds.count();

	if (subsecondDigits > 9) {
		subsecondDigits = 9;
	}
	if (subsecondDigits < 0) {
		subsecondDigits = 0;
	}

	int subsecondsLen = subsecondDigits + ((subsecondDigits + 2) / 3);

	sprintf(subsecondsStr, ".%3.3li.%3.3li.%3.3li", ns / 1000000,
			(ns / 1000) % 1000, ns % 1000);

	subsecondsStr[subsecondsLen] = 0;

	std::time_t now_c = std::chrono::system_clock::to_time_t(_date);
	auto loc = std::localtime(&now_c);

	sdate.resize(15);
	std::strftime(sdate.data(), sdate.size() + 1, "%F", loc);
	sdate.resize(strlen(sdate.c_str()));

	stime.resize(15);
	std::strftime(stime.data(), stime.size() + 1, "%T", loc);
	stime.resize(strlen(stime.c_str()));
	stime += subsecondsStr;
}
}
}
