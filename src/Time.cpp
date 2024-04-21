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
#include <ctime>

#include <chrono>

#include "../include/icon7/Time.hpp"

namespace icon7
{
namespace time
{
struct DayTimeBegin {
	DayTimeBegin()
	{
		date = std::chrono::utc_clock::now();
		time = std::chrono::steady_clock::now();
		nanosecondsSincEpoch =
			std::chrono::duration_cast<std::chrono::nanoseconds, int64_t>(
				date.time_since_epoch())
				.count();
		
		time_t secs;
		secs = std::time(nullptr);
		secs -= secs%3600;
		secs += 1800;
		
		struct tm local = *localtime(&secs);
		struct tm utc = *gmtime(&secs);
		defaultGmTimeTm = utc;
		
		printf("gtoff: %i\n", (int)local.tm_gmtoff);
		
		int diff = utc.tm_hour - local.tm_hour;
		bool c2 = false;
		if (local.tm_year<utc.tm_year) {
			printf("1c1\n");
			c2 = true;
		} else if (local.tm_year == utc.tm_year) {
			if (local.tm_yday<utc.tm_yday) {
				printf("2c1\n");
					c2 = true;
			} else if (local.tm_yday == utc.tm_yday) {
				if (local.tm_hour<utc.tm_hour) {
					printf("3c1\n");
					c2 = true;
				} else {
					printf("4c1\n");
				}
			} else {
				if (local.tm_hour<utc.tm_hour) {
					printf("5c1\n");
					diff += 24;
				} else {
					printf("6c1\n");
				}
			}
		} else {
			printf("7c1\n");
		}
		
		
		if (c2) {
			printf("\n\nC2\n");
		} else {
			printf("\n\nC1\n");
		}
		
		diff = -local.tm_gmtoff / 3600;
		hoursOffset = diff;
		hoursOffsetNanoseconds = hoursOffset*3600ll*1000ll*1000ll*1000ll;
		
		printf("timezone: UTC%+i\n\n", (int)hoursOffset);
	}
	std::chrono::utc_clock::time_point date;
	std::chrono::steady_clock::time_point time;
	uint64_t nanosecondsSincEpoch;
	int64_t hoursOffset;
	int64_t hoursOffsetNanoseconds;
	std::tm defaultGmTimeTm;
};

static const DayTimeBegin glob;

uint64_t GetTimestamp()
{
	auto now = std::chrono::steady_clock::now();

	auto diff = now - glob.time;
	int64_t ns =
		std::chrono::duration_cast<std::chrono::nanoseconds, int64_t>(diff)
			.count();

	return glob.nanosecondsSincEpoch + (uint64_t)ns;
}

int IsLeapYear(int year) {
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int LeapYearsBefore(int year) {
	return (year / 4) - (year / 100) + (year / 400);
}

struct YMD
{
	int year;
	int month;
	int day;
};

void YMDFromTimestamp(uint64_t timestamp, int &day, int &month, int &year)
{
	int days = timestamp / (24ll*3600ll*1000ll*1000ll*1000ll);
	
	year = 1970 + (days-370)/366;
	int leapYears = LeapYearsBefore(year-1) - LeapYearsBefore(1970);
	
	days -= (year-1970)*365 + leapYears;
	bool isLeap = false;
	while (true) {
		isLeap = IsLeapYear(year);
		int max = 365;
		if (isLeap) {
			max = 366;
		}
		if (days >= max) {
			days -= max;
			year++;
		} else {
			break;
		}
	}
	int feb = isLeap ? 29 : 28;
	const int daysPerMonth[12] = {31, feb, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	month = 0;
	for (; days >= daysPerMonth[month];) {
		days -= daysPerMonth[month];
		month++;
	}
	month++;
	day = days+1;
}

YMD YMDFromTimestamp(uint64_t timestamp)
{
	YMD ymd;
	YMDFromTimestamp(timestamp, ymd.day, ymd.month, ymd.year);
	return ymd;
}

uint64_t TimestampFromYMD(int day, int month, int year)
{
	int64_t days = 0;
	int leapYears = LeapYearsBefore(year-1) - LeapYearsBefore(1970);
	
	int feb = IsLeapYear(year) ? 29 : 28;
	const int daysPerMonth[12] = {31, feb, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	days += (year-1970) * 365 + day-1 + leapYears;
	
	month--;
	for (int i=0; i<month; ++i) {
		days += daysPerMonth[i];
	}
	
	return days*24ll*3600ll*1000ll*1000ll*1000ll;
}
uint64_t TimestampFromYMD(YMD ymd)
{
	return TimestampFromYMD(ymd.day, ymd.month, ymd.year);
}


std::string TimestampToString2(uint64_t timestamp, int subsecondDigits)
{
	timestamp -= glob.hoursOffsetNanoseconds;
	
	
	const std::chrono::nanoseconds nanoseconds(timestamp);
	const std::chrono::seconds seconds =
		std::chrono::duration_cast<std::chrono::seconds, int64_t>(nanoseconds);
	const std::chrono::nanoseconds subseconds = nanoseconds - seconds;

	char subsecondsStr[32];
	const uint64_t ns = subseconds.count();

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
	
	YMD ymd = YMDFromTimestamp(timestamp);
	int h=(seconds.count()/3600)%24, m=(seconds.count()/60)%60, s=seconds.count()%60;

	std::string str;
	str.resize(48);
	
	sprintf(str.data(), "%4.4i-%2.2i-%2.2i+%2.2i:%2.2i:%2.2i", ymd.year,
			ymd.month, ymd.day, h, m, s);
	str.resize(strlen(str.c_str()));

	str += subsecondsStr;
	char tmp[32];
	sprintf(tmp, "UTC%+i", (int)glob.hoursOffset);
	str += tmp;
	return str;
}

std::string TimestampToString(uint64_t timestamp, int subsecondDigits)
{
	return "XXX";
	timestamp -= glob.hoursOffsetNanoseconds;
	const std::chrono::nanoseconds nanoseconds(timestamp);
	const std::chrono::seconds seconds =
		std::chrono::duration_cast<std::chrono::seconds, int64_t>(nanoseconds);
	const std::chrono::nanoseconds subseconds = nanoseconds - seconds;

	char subsecondsStr[32];
	const uint64_t ns = subseconds.count();

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

	std::chrono::system_clock::time_point _date(seconds);
	std::time_t now_c = std::chrono::system_clock::to_time_t(_date);
	std::tm loc;
	loc = *gmtime_r(&now_c, &loc);

	std::string str;
	str.resize(48);

	str.resize(
		std::strftime(str.data(), str.size() + 1, "%Y-%m-%d+%H:%M:%S", &loc));
	str += subsecondsStr;
	char tmp[32];
	sprintf(tmp, "UTC%+i", (int)glob.hoursOffset);
	str += tmp;
	return str;
}

std::string GetCurrentTimestampString(int subsecondDigits)
{
	return TimestampToString2(GetTimestamp(), subsecondDigits);
}

uint64_t StringToTimestamp(const std::string &str)
{
	int years = 0, months = 0, days = 0, hours = 0, minutes = 0, seconds = 0;
	sscanf(str.c_str(), "%i-%i-%i+%i:%i:%i", &years, &months, &days, &hours,
		   &minutes, &seconds);
	std::string subsecondsStr = str.c_str() + 20;
	auto utcPos = subsecondsStr.find("UTC");
	int utcOffset = 0;
	if (utcPos != std::string::npos) {
		std::string utcStr = subsecondsStr.c_str()+utcPos;
		subsecondsStr.resize(utcPos);
		sscanf(utcStr.c_str(), "UTC%i", &utcOffset);
		printf("Read utc offset: %i   <- `%s`\n", utcOffset, utcStr.c_str());
	}
	if (subsecondsStr.size() > 7)
		subsecondsStr.erase(7, 1);
	if (subsecondsStr.size() > 3)
		subsecondsStr.erase(3, 1);
	subsecondsStr.resize(9, '0');
	uint64_t subseconds = 0;
	sscanf(subsecondsStr.c_str(), "%li", &subseconds);

	std::tm tp = glob.defaultGmTimeTm;
	tp.tm_year = years - 1900;
	tp.tm_mon = months - 1;
	tp.tm_mday = days;
	tp.tm_hour = hours;
	tp.tm_min = minutes;
	tp.tm_sec = seconds;
	tp.tm_gmtoff = 3600*utcOffset;
	std::time_t t = std::mktime(&tp);
	t = std::mktime(&tp);
	
	t = TimestampFromYMD({years, months, days})/(1000ll*1000ll*1000ll) + (hours*60+minutes)*60+seconds;
	
	printf("decode gmt off: %i\n", (int)tp.tm_gmtoff);
	

	int64_t tpSeconds =
		std::chrono::duration_cast<std::chrono::seconds, int64_t>(
			std::chrono::system_clock::from_time_t(t).time_since_epoch())
			.count();

	tpSeconds += utcOffset * 3600ll;
// 	tpSeconds += 3600;
	uint64_t ns = ((uint64_t)tpSeconds) * 1000000000llu + subseconds;
	printf("Reconstructed timestamp: %s\n", TimestampToString2(ns,9).c_str());
	return ns;
}
} // namespace time
} // namespace icon7
