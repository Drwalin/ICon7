// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
		timeSinceEpoch = time.time_since_epoch().count();
		nanosecondsSinceEpoch =
			std::chrono::duration_cast<std::chrono::nanoseconds, int64_t>(
				date.time_since_epoch())
				.count() -
			timeSinceEpoch;

		time_t secs;
		secs = std::time(nullptr);
		struct tm local = *localtime(&secs);

#ifdef tm_gmtoff
		hoursOffset = -local.tm_gmtoff / 3600;
#else
		hoursOffset = 0;
#endif
		hoursOffsetNanoseconds =
			hoursOffset * 3600ll * 1000ll * 1000ll * 1000ll;
	}
	std::chrono::utc_clock::time_point date;
	std::chrono::steady_clock::time_point time;
	uint64_t timeSinceEpoch;
	uint64_t nanosecondsSinceEpoch;
	int64_t hoursOffset;
	int64_t hoursOffsetNanoseconds;
};

static const DayTimeBegin glob;

uint64_t GetTimestamp()
{
	return TemporaryTimestampToTimestamp(GetTemporaryTimestamp());
}

uint64_t GetTemporaryTimestamp()
{
	return std::chrono::steady_clock::now().time_since_epoch().count();
}

uint64_t TemporaryTimestampToTimestamp(uint64_t tmpTimestamp)
{
	return glob.nanosecondsSinceEpoch + tmpTimestamp;
}

int64_t DeltaNsBetweenTimestamps(uint64_t begin, uint64_t end)
{
	return end - begin;
}

double DeltaSecBetweenTimestamps(uint64_t begin, uint64_t end)
{
	return NanosecondsToSeconds(DeltaNsBetweenTimestamps(begin, end));
}

double DeltaMSecBetweenTimestamps(uint64_t begin, uint64_t end)
{
	return DeltaNsBetweenTimestamps(begin, end) / (1000.0 * 1000.0);
}

double DeltaUSecBetweenTimestamps(uint64_t begin, uint64_t end)
{
	return DeltaNsBetweenTimestamps(begin, end) / 1000.0;
}

double NanosecondsToSeconds(int64_t ns)
{
	return ns / (1000.0 * 1000.0 * 1000.0);
}

int IsLeapYear(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int LeapYearsBefore(int year)
{
	return (year / 4) - (year / 100) + (year / 400);
}

struct YMD {
	int year;
	int month;
	int day;
};

void YMDFromTimestamp(uint64_t timestamp, int &day, int &month, int &year)
{
	int days = timestamp / (24ll * 3600ll * 1000ll * 1000ll * 1000ll);

	year = 1970 + (days - 370) / 366;
	int leapYears = LeapYearsBefore(year - 1) - LeapYearsBefore(1970);

	days -= (year - 1970) * 365 + leapYears;
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
	const int daysPerMonth[12] = {31, feb, 31, 30, 31, 30,
								  31, 31,  30, 31, 30, 31};

	month = 0;
	for (; days >= daysPerMonth[month];) {
		days -= daysPerMonth[month];
		month++;
	}
	month++;
	day = days + 1;
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
	int leapYears = LeapYearsBefore(year - 1) - LeapYearsBefore(1970);

	int feb = IsLeapYear(year) ? 29 : 28;
	const int daysPerMonth[12] = {31, feb, 31, 30, 31, 30,
								  31, 31,  30, 31, 30, 31};

	days += (year - 1970) * 365 + day - 1 + leapYears;

	month--;
	for (int i = 0; i < month; ++i) {
		days += daysPerMonth[i];
	}

	return days * 24ll * 3600ll * 1000ll * 1000ll * 1000ll;
}
uint64_t TimestampFromYMD(YMD ymd)
{
	return TimestampFromYMD(ymd.day, ymd.month, ymd.year);
}

std::string TimestampToString(uint64_t timestamp, int subsecondDigits)
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
	int h = (seconds.count() / 3600) % 24, m = (seconds.count() / 60) % 60,
		s = seconds.count() % 60;

	std::string str;
	str.resize(48);

	sprintf(str.data(), "%4.4i-%2.2i-%2.2i+%2.2i:%2.2i:%2.2i", ymd.year,
			ymd.month, ymd.day, h, m, s);
	str.resize(strlen(str.c_str()));

	str += subsecondsStr;
	char tmp[32];
	sprintf(tmp, "U%+i", -(int)glob.hoursOffset);
	str += tmp;
	return str;
}

std::string GetCurrentTimestampString(int subsecondDigits)
{
	return TimestampToString(GetTimestamp(), subsecondDigits);
}

uint64_t StringToTimestamp(std::string str)
{
	int years = 0, months = 0, days = 0, hours = 0, minutes = 0, seconds = 0;
	sscanf(str.c_str(), "%d-%d-%d+%d:%d:%d", &years, &months, &days, &hours,
		   &minutes, &seconds);
	std::string subsecondsStr = str.c_str() + 20;
	auto utcPos = subsecondsStr.find("U");
	int utcOffset = 0;
	if (utcPos != std::string::npos) {
		std::string utcStr = subsecondsStr.c_str() + utcPos;
		subsecondsStr.resize(utcPos);
		sscanf(utcStr.c_str(), "U%d", &utcOffset);
		utcOffset = -utcOffset;
	}
	if (subsecondsStr.size() > 7)
		subsecondsStr.erase(7, 1);
	if (subsecondsStr.size() > 3)
		subsecondsStr.erase(3, 1);
	subsecondsStr.resize(9, '0');
	int64_t subseconds = atoi(subsecondsStr.c_str());

	int64_t t =
		TimestampFromYMD({years, months, days}) / (1000ll * 1000ll * 1000ll) +
		(hours * 60 + minutes) * 60 + seconds;

	int64_t tpSeconds =
		std::chrono::duration_cast<std::chrono::seconds, int64_t>(
			std::chrono::system_clock::from_time_t(t).time_since_epoch())
			.count();

	tpSeconds += utcOffset * 3600ll;
	uint64_t ns = ((uint64_t)tpSeconds) * 1000000000llu + subseconds;
	return ns;
}
} // namespace time
} // namespace icon7
