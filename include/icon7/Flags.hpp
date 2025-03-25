// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
