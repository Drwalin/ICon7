// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Time.hpp"

#if ICON7_USE_RPMALLOC
#include "../include/icon7/Debug.hpp"
#include "../rpmalloc/rpmalloc/rpmalloc.h"
#else
#include <cstdlib>
#endif

#include "../include/icon7/MemoryPool.hpp"

namespace icon7
{
MemoryStats::MemoryStats() { startTimestamp = time::GetTimestamp(); }

MemoryStats MemoryPool::stats = {};

AllocatedObject<void> MemoryPool::Allocate(size_t bytes)
{
#if ICON7_USE_RPMALLOC
	static int ____staticInitilizeRpmalloc = (rpmalloc_initialize(), 0);
	class __RpMallocThreadLocalDestructor final
	{
	public:
		__RpMallocThreadLocalDestructor() { rpmalloc_thread_initialize(); }
		~__RpMallocThreadLocalDestructor() { rpmalloc_thread_finalize(1); }
	};
	thread_local __RpMallocThreadLocalDestructor
		____staticThreadLocalDestructorRpmalloc;

	void *ptr = rpmalloc(bytes);
	size_t _bytes = rpmalloc_usable_size(ptr);
	if (_bytes < bytes) {
		rpfree(ptr);
		LOG_FATAL("Allocated memory is smaller than requested");
		return {nullptr, 0};
	} else {
		bytes = _bytes;
	}

	int64_t max = (stats.allocatedBytes += bytes) - stats.deallocatedBytes;
	{
		int64_t v = stats.maxInUseAtOnce;
		while (max > v) {
			if (stats.maxInUseAtOnce.compare_exchange_weak(
					v, max, std::memory_order_release,
					std::memory_order_relaxed)) {
				break;
			}
		}
	}

	if (bytes <= MemoryStats::MAX_BYTES_FOR_SMALL_ALLOCATIONS) {
		stats.smallAllocations += 1;
	} else if (bytes <= MemoryStats::MAX_BYTES_FOR_MEDIUM_ALLOCATIONS) {
		stats.mediumAllocations += 1;
	} else {
		stats.largeAllocations += 1;
	}

	return {ptr, _bytes};
#else
	stats.allocatedBytes += bytes;
	if (bytes <= MemoryStats::MAX_BYTES_FOR_SMALL_ALLOCATIONS) {
		stats.smallAllocations += 1;
	} else if (bytes <= MemoryStats::MAX_BYTES_FOR_MEDIUM_ALLOCATIONS) {
		stats.mediumAllocations += 1;
	} else {
		stats.largeAllocations += 1;
	}

	return {malloc(bytes), bytes};
#endif
}

void MemoryPool::Release(void *ptr, size_t bytes)
{
#if ICON7_USE_RPMALLOC
	bytes = rpmalloc_usable_size(ptr);

	stats.deallocatedBytes += bytes;
	if (bytes <= MemoryStats::MAX_BYTES_FOR_SMALL_ALLOCATIONS) {
		stats.smallDeallocations += 1;
	} else if (bytes <= MemoryStats::MAX_BYTES_FOR_MEDIUM_ALLOCATIONS) {
		stats.mediumDeallocations += 1;
	} else {
		stats.largeDeallocations += 1;
	}

	rpfree(ptr);
	return;
#else
	stats.deallocations -= 1;
	free(ptr);
#endif
}
} // namespace icon7
