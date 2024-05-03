/*
 *  This file is part of ICon7.
 *  Copyright (C) 2024 Marek Zalewski aka Drwalin
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

#include <memory>

#include "../../concurrentqueue/concurrentqueue.h"

namespace icon7
{
namespace memory_internal
{
class alignas(64) BigChunks final
{
public:
	BigChunks(size_t BLOCK_SIZE, size_t maxResidentChunks);
	~BigChunks();

	void *Acquaire();
	void Release(void *ptr);

	inline uint32_t GetBlockSize() const { return BLOCK_SIZE; }

private:
	alignas(64) moodycamel::ConcurrentQueue<std::unique_ptr<uint8_t[]>> queue;

	alignas(64) size_t maxResidentChunks;
	const size_t BLOCK_SIZE;
};

class alignas(64) SmallChunks final
{
public:
	SmallChunks(size_t BLOCK_SIZE, size_t DEFAULT_BLOCKS_ALLOCATED,
				size_t maxResidentChunks);
	~SmallChunks();

	void *Acquaire();
	void Release(void *ptr);

	inline uint32_t GetBlockSize() const { return BLOCK_SIZE; }

private:
	bool IsPreallocated(void *ptr);

private:
	alignas(64) moodycamel::ConcurrentQueue<void *> queue;

	alignas(64) size_t maxResidentChunks;

	void *defaultAllocated;
	void *defaultEndAllocated;

	const size_t BLOCK_SIZE;
	const size_t BLOCKS_ALLOCATED;
};
} // namespace memory_internal

class MemoryPool final
{
public:
	MemoryPool(uint32_t defaultCapacityPerSize, uint32_t bigObjectsLimit);
	~MemoryPool();

	static uint32_t GetNearestFittingSize(uint32_t bytes);

	void *_Allocate(uint32_t bytes);
	void _Release(void *ptr, uint32_t bytes);

	template <typename T, typename... Args> T *_AllocateTyped(Args &&...args)
	{
		return new (_Allocate(sizeof(T))) T(std::move(args)...);
	}

	template <typename T> void _ReleaseTyped(T *ptr, uint32_t bytes)
	{
		ptr->~T();
		_Release(ptr, bytes);
	}

public:
	static MemoryPool defaultMemoryPool;

	static void *Allocate(uint32_t bytes);
	static void Release(void *ptr, uint32_t bytes);

	template <typename T, typename... Args>
	static T *AllocateTyped(Args &&...args)
	{
		Allocate<T>(std::move(args)...);
	}

	template <typename T> static void ReleaseTyped(T *ptr, uint32_t bytes)
	{
		Release(ptr, bytes);
	}

private:
	memory_internal::SmallChunks poolsOfSmall[6];
	memory_internal::BigChunks poolsOfBig[5];
};
} // namespace icon7

#endif
