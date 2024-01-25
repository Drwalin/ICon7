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

#include <cstring>

#include <random>

#include "../include/icon7/Random.hpp"

namespace icon7
{
static std::random_device rd;
static std::mt19937_64 mt(rd());
uint64_t Random::UInt64()
{
	return mt();
}

void Random::Bytes(void *_ptr, std::size_t bytes)
{
	uint8_t *ptr = (uint8_t *)_ptr;
	for (; bytes >= 8; bytes -= 8, ptr += 8) {
		*(uint64_t*)ptr = mt();
	}
	if (bytes) {
		uint64_t v = mt();
		memcpy(ptr, &v, bytes);
	}
}
}

