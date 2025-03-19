/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
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

#ifndef ICON7_FLAGS_HPP
#define ICON7_FLAGS_HPP

#include <cstdint>

namespace icon7
{
using Flags = uint32_t;

enum FlagsValues : uint32_t {
	FLAG_UNRELIABLE = 0,
	FLAG_RELIABLE = 1,

	FLAGS_CALL_NO_FEEDBACK = 0,
	FLAGS_CALL = 2,
	FLAGS_CALL_RETURN_FEEDBACK = 4,
	FLAGS_PROTOCOL_CONTROLL_SEQUENCE = 6,
};

static_assert(sizeof(Flags) == 4);

enum IPProto { IPinvalid = 0, IPv4 = 4, IPv6 = 6 };
} // namespace icon7

#endif
