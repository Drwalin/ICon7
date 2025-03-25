// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_RANDOM_HPP
#define ICON7_RANDOM_HPP

#include <cstdint>

namespace icon7
{
class Random
{
public:
	static uint64_t UInt64();
	static void Bytes(void *ptr, std::size_t bytes);
};
} // namespace icon7

#endif
