/*
 *  This file is part of ICon7.
 *  Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
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

#ifndef ICON7_MEMORY_POOL_HPP
#define ICON7_MEMORY_POOL_HPP

#include <cstdint>
#include <cstdio>

#include <utility>

namespace icon7
{
template <typename T> struct AllocatedObject {
	T *object;
	size_t capacity;
};

class MemoryPool
{
public:
	MemoryPool() {};
	~MemoryPool() {};

	__attribute__((noinline)) static AllocatedObject<void>
	Allocate(size_t bytes);
	__attribute__((noinline)) static void Release(void *ptr, size_t bytes);

	template <typename T, typename... Args>
	static T *AllocateTyped(Args &&...args)
	{
		AllocatedObject<void> v = Allocate(sizeof(T));
		AllocatedObject<T> ret;
		ret.capacity = v.capacity;
		ret.object = new (v.object) T(std::move(args)...);
		return ret.object;
	}

	template <typename T> static void ReleaseTyped(T *ptr, size_t bytes)
	{
		ptr->~T();
		Release(ptr, bytes);
	}

	static void PrintStats();
};
} // namespace icon7

#endif
