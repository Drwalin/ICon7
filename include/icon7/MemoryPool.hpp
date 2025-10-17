// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_MEMORY_POOL_HPP
#define ICON7_MEMORY_POOL_HPP

#include <cstdio>

#include <utility>

#include "Stats.hpp"

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

	static MemoryStats &Stats();
};
} // namespace icon7

#endif
