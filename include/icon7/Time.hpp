// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_TIME_HPP
#define ICON7_TIME_HPP

#include <cstdint>

#include <string>

namespace icon7
{
namespace time
{
int64_t GetTemporaryTimestamp();
int64_t TemporaryTimestampToTimestamp(int64_t tmpTimestamp);
int64_t GetTimestamp();

int64_t DeltaNsBetweenTimestamps(int64_t begin, int64_t end);
double DeltaSecBetweenTimestamps(int64_t begin, int64_t end);
double DeltaMSecBetweenTimestamps(int64_t begin, int64_t end);
double DeltaUSecBetweenTimestamps(int64_t begin, int64_t end);
double NanosecondsToSeconds(int64_t ns);

std::string TimestampToString(int64_t timestamp, int subsecondsDigits);
std::string GetCurrentTimestampString(int subsecondsDigits);
int64_t StringToTimestamp(std::string str);
void YMDFromTimestamp(int64_t timestamp, int &day, int &month, int &year);
int64_t TimestampFromYMD(int day, int month, int year);

void SleepNSec(int64_t nanoseconds);
void SleepUSec(int64_t microseconds);
void SleepMSec(int64_t milliseconds);
void SleepSec(double seconds);
} // namespace time
} // namespace icon7

#endif
