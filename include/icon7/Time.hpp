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
uint64_t GetTimestamp();
std::string TimestampToString(uint64_t timestamp, int subsecondsDigits);
std::string GetCurrentTimestampString(int subsecondsDigits);
uint64_t StringToTimestamp(std::string str);
void YMDFromTimestamp(uint64_t timestamp, int &day, int &month, int &year);
uint64_t TimestampFromYMD(int day, int month, int year);
} // namespace time

} // namespace icon7

#endif
