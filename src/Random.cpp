// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstring>

#include <random>

#include "../include/icon7/Random.hpp"

namespace icon7
{
static std::random_device rd;
static std::mt19937_64 mt(rd());
uint64_t Random::UInt64() { return mt(); }

void Random::Bytes(void *_ptr, std::size_t bytes)
{
	uint8_t *ptr = (uint8_t *)_ptr;
	for (; bytes >= 8; bytes -= 8, ptr += 8) {
		*(uint64_t *)ptr = mt();
	}
	if (bytes) {
		uint64_t v = mt();
		memcpy(ptr, &v, bytes);
	}
}
} // namespace icon7
