// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_TIME_HPP
#define ICON7_TIME_HPP

#include <cstdint>

#include <string>

#include "../../concurrent/time.hpp"

namespace icon7
{
namespace time
{
using namespace concurrent::time;
using Point = point;
using Diff = diff;

struct Timestamp {
	int64_t ns;
};

Point GetTemporaryTimestamp();
Timestamp TemporaryTimestampToTimestamp(point tmpTimestamp);
Timestamp GetTimestamp();

double DeltaSecBetweenTimestamps(Timestamp begin, Timestamp end);
double DeltaMSecBetweenTimestamps(Timestamp begin, Timestamp end);
double DeltaUSecBetweenTimestamps(Timestamp begin, Timestamp end);
Diff DeltaNsBetweenTimestamps(Timestamp begin, Timestamp end);
double NanosecondsToSeconds(Diff ns);

double DeltaSecBetweenTimepoints(Point begin, Point end);
double DeltaMSecBetweenTimepoints(Point begin, Point end);
double DeltaUSecBetweenTimepoints(Point begin, Point end);
Diff DeltaNsBetweenTimepoints(Point begin, Point end);

std::string TimestampToString(Timestamp timestamp, int subsecondsDigits);
std::string GetCurrentTimestampString(int subsecondsDigits);
Timestamp StringToTimestamp(std::string str);
void YMDFromTimestamp(Timestamp timestamp, int &day, int &month, int &year);
Timestamp TimestampFromYMD(int day, int month, int year);

void Sleep(Diff timediff);
} // namespace time
} // namespace icon7

#endif
