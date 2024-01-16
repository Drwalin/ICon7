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

#ifndef ICON7_UTIL_HPP
#define ICON7_UTIL_HPP

namespace icon7
{
template <typename... Args>
auto ConvertLambdaToFunctionPtr(void (*fun)(Args...))
{
	return fun;
}

template <typename Fun> auto ConvertLambdaToFunctionPtr(Fun &&fun)
{
	return +fun;
}
} // namespace icon7

#endif
