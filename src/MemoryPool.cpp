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

#include <cstdlib>

#include <bit>

#include "../include/icon7/Debug.hpp"

#include "../include/icon7/MemoryPool.hpp"

namespace icon7
{
namespace memory_internal
{
BigChunks::BigChunks(size_t BLOCK_SIZE, size_t maxResidentChunks)
	: queue(maxResidentChunks), maxResidentChunks(maxResidentChunks),
	  BLOCK_SIZE(BLOCK_SIZE)
{
}

BigChunks::~BigChunks() {}

void *BigChunks::Acquaire()
{
	std::unique_ptr<uint8_t[]> ptr;
	if (queue.try_dequeue(ptr)) {
		return ptr.release();
	} else {
		return malloc(BLOCK_SIZE);
	}
}

void BigChunks::Release(void *ptr)
{
	if (queue.size_approx() > maxResidentChunks) {
		free(ptr);
	} else {
		queue.enqueue(std::unique_ptr<uint8_t[]>((uint8_t *)ptr));
	}
}

SmallChunks::SmallChunks(size_t BLOCK_SIZE, size_t DEFAULT_BLOCKS_ALLOCATED,
						 size_t maxResidentChunks)
	: queue(maxResidentChunks * 16), maxResidentChunks(maxResidentChunks),
	  BLOCK_SIZE(BLOCK_SIZE), BLOCKS_ALLOCATED(DEFAULT_BLOCKS_ALLOCATED)
{
	defaultAllocated = malloc(BLOCK_SIZE * DEFAULT_BLOCKS_ALLOCATED);
	for (int i = 0; i < DEFAULT_BLOCKS_ALLOCATED; ++i) {
		queue.enqueue((uint8_t *)defaultAllocated + BLOCK_SIZE * i);
	}
	defaultEndAllocated =
		(uint8_t *)defaultAllocated + BLOCK_SIZE * DEFAULT_BLOCKS_ALLOCATED;
}

SmallChunks::~SmallChunks()
{
	const uint32_t maxSize = 128;
	void *ptrs[maxSize];
	for (int i = 0; i < 16; ++i) {
		while (queue.size_approx() > 0) {
			uint32_t dequeued = queue.try_dequeue_bulk(ptrs, maxSize);
			for (uint32_t j = 0; j < dequeued; ++j) {
				void *ptr = ptrs[i];
				if (!IsPreallocated(ptr)) {
					free(ptr);
				}
			}
		}
	}
	free(defaultAllocated);
	defaultAllocated = nullptr;
	defaultEndAllocated = nullptr;
}

void *SmallChunks::Acquaire()
{
	void *ptr = nullptr;
	if (queue.try_dequeue(ptr)) {
		return ptr;
	} else {
		return malloc(BLOCK_SIZE);
	}
}

void SmallChunks::Release(void *ptr)
{
	if (IsPreallocated(ptr)) {
		queue.enqueue(ptr);
	} else if (queue.size_approx() > maxResidentChunks) {
		free(ptr);
	} else {
		queue.enqueue(ptr);
	}
}

bool SmallChunks::IsPreallocated(void *ptr)
{
	if (defaultAllocated <= ptr && ptr < defaultEndAllocated) {
		return true;
	}
	return false;
}
} // namespace memory_internal

using namespace memory_internal;

MemoryPool::MemoryPool(uint32_t dc, uint32_t bigObjectsLimit)
	: poolsOfSmall({64, dc / 64, 1024 * 1024}, {128, dc / 128, 512 * 1024},
				   {256, dc / 256, 256 * 1024}, {512, dc / 512, 128 * 1024},
				   {1024, dc / 1024, 64 * 1024}, {2048, dc / 2048, 32 * 1024}),
	  poolsOfBig({1024 * 4, bigObjectsLimit}, {1024 * 8, bigObjectsLimit},
				 {1024 * 16, bigObjectsLimit}, {1024 * 32, bigObjectsLimit},
				 {1024 * 64, bigObjectsLimit})
{
}

MemoryPool::~MemoryPool() {}

uint32_t MemoryPool::GetNearestFittingSize(uint32_t bytes)
{
	if (bytes > 64 * 1024) {
		return bytes;
	}
	bytes = std::bit_ceil(bytes);
	if (bytes < 64) {
		return 64;
	}
	return bytes;
}

void *MemoryPool::_Allocate(uint32_t bytes)
{
	if (bytes > 64 * 1024) {
		return malloc(bytes);
	}
	bytes = GetNearestFittingSize(bytes);
	int32_t bits = std::bit_width(bytes) - 7;

	if (bits < 6) {
		return poolsOfSmall[bits].Acquaire();
	} else if (bits < 11) {
		return poolsOfBig[bits - 6].Acquaire();
	} else {
		LOG_FATAL("Failed to recognize size of object");
		return malloc(bytes);
	}
	return nullptr;
}

void MemoryPool::_Release(void *ptr, uint32_t bytes)
{
	if (bytes > 64 * 1024) {
		free(ptr);
		return;
	}
	bytes = GetNearestFittingSize(bytes);
	int32_t bits = std::bit_width(bytes) - 7;

	if (bits < 6) {
		poolsOfSmall[bits].Release(ptr);
	} else if (bits < 11) {
		poolsOfBig[bits - 6].Release(ptr);
	} else {
		LOG_FATAL("Failed to recognize size of object");
		free(ptr);
	}
}

void *MemoryPool::Allocate(uint32_t bytes)
{
	return defaultMemoryPool._Allocate(bytes);
}

void MemoryPool::Release(void *ptr, uint32_t bytes)
{
	defaultMemoryPool._Release(ptr, bytes);
}

MemoryPool MemoryPool::defaultMemoryPool{1024 * 64, 1024 * 64};

} // namespace icon7
