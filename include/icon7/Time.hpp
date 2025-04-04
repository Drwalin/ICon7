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
uint64_t GetTemporaryTimestamp();
uint64_t TemporaryTimestampToTimestamp(uint64_t tmpTimestamp);
uint64_t GetTimestamp();
int64_t DeltaNsBetweenTimestamps(uint64_t begin, uint64_t end);
double DeltaSecBetweenTimestamps(uint64_t begin, uint64_t end);
double DeltaMSecBetweenTimestamps(uint64_t begin, uint64_t end);
double DeltaUSecBetweenTimestamps(uint64_t begin, uint64_t end);
double NanosecondsToSeconds(int64_t ns);
std::string TimestampToString(uint64_t timestamp, int subsecondsDigits);
std::string GetCurrentTimestampString(int subsecondsDigits);
uint64_t StringToTimestamp(std::string str);
void YMDFromTimestamp(uint64_t timestamp, int &day, int &month, int &year);
uint64_t TimestampFromYMD(int day, int month, int year);
} // namespace time

} // namespace icon7

#endif
