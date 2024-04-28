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
