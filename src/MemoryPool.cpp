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

#if !defined(ICON7_USE_RPMALLOC) and !defined(ICON7_USE_THREAD_CACHED_POOL)
// Current default configuration
#define ICON7_USE_THREAD_CACHED_POOL 0
#define ICON7_USE_RPMALLOC 0
#endif

#if !defined(ICON7_USE_RPMALLOC) and !defined(ICON7_USE_THREAD_CACHED_POOL)
#if !defined(ICON7_USE_RPMALLOC)
#define ICON7_USE_RPMALLOC 1
#endif
#elif !defined(ICON7_USE_THREAD_CACHED_POOL)
#define ICON7_USE_THREAD_CACHED_POOL 1
#endif

#if !defined(ICON7_USE_RPMALLOC) or !ICON7_USE_RPMALLOC
#define ICON7_CUSTOM_ALLOCATOR
#endif

#if ICON7_USE_RPMALLOC
#include "../rpmalloc/rpmalloc/rpmalloc.h"
#else
#include <bit>

#include "../concurrentqueue/concurrentqueue.h"

#include "../include/icon7/ConcurrentQueueTraits.hpp"
#endif

#if ICON7_MEMORY_COLLECT_STATS
#include <atomic>
#endif

#if ICON7_USE_MALLOC_TRIM
#include <malloc.h>
#endif

#include <cstdio>

#include "../include/icon7/MemoryPool.hpp"

namespace icon7
{
#if ICON7_USE_MALLOC_TRIM
#define ICON7_MALLOC_TRIM() MallocTrim()
static void MallocTrim()
{
	static uint32_t i = 0;
	if (((++i) & 1023) == 63) {
		malloc_trim(0);
	}
}
#else
#define ICON7_MALLOC_TRIM()
#endif

#if ICON7_MEMORY_COLLECT_STATS
alignas(64) std::atomic<uint64_t> acquiredObjects = 0;
alignas(64) std::atomic<uint64_t> releasedObjects = 0;
alignas(64) std::atomic<uint64_t> acquiredMemory = 0;
alignas(64) std::atomic<uint64_t> releasedMemory = 0;
alignas(64) std::atomic<uint64_t> allocatedObjects = 0;
alignas(64) std::atomic<uint64_t> freedObjects = 0;
alignas(64) std::atomic<uint64_t> allocatedMemory = 0;
alignas(64) std::atomic<uint64_t> freedMemory = 0;
void MemoryPool::PrintStats()
{
	printf("Objects  acquired: +%16lu -%16lu = %16lu\n", acquiredObjects.load(),
		   releasedObjects.load(),
		   acquiredObjects.load() - releasedObjects.load());
	printf("Memory   acquired: +%16.2f -%16.2f = %16.2f MiB\n",
		   acquiredMemory.load() / (1024.0 * 1024.0),
		   releasedMemory.load() / (1024.0 * 1024.0),
		   acquiredMemory.load() / (1024.0 * 1024.0) -
			   releasedMemory.load() / (1024.0 * 1024.0));
	printf("Objects allocated: +%16lu -%16lu = %16lu\n",
		   allocatedObjects.load(), freedObjects.load(),
		   allocatedObjects.load() - freedObjects.load());
	printf("Memory  allocated: +%16.2f -%16.2f = %16.2f MiB\n",
		   allocatedMemory.load() / (1024.0 * 1024.0),
		   freedMemory.load() / (1024.0 * 1024.0),
		   allocatedMemory.load() / (1024.0 * 1024.0) -
			   freedMemory.load() / (1024.0 * 1024.0));
	fflush(stdout);
}
#else
void MemoryPool::PrintStats()
{
	printf("Memory stats not collected\n");
	fflush(stdout);
}
#endif

#if defined(ICON7_CUSTOM_ALLOCATOR)
const static uint32_t NUMBER_OF_POOLS = 8;
const static uint32_t SMALLES_OBJECT_SIZE = 64;
const static uint32_t BIGGEST_OBJECT_SIZE = SMALLES_OBJECT_SIZE
											<< (NUMBER_OF_POOLS - 1);

const static uint32_t objectSizes[NUMBER_OF_POOLS] = {64,	128,  256,	512,
													  1024, 2048, 4096, 8192};
#if ICON7_USE_THREAD_CACHED_POOL
#else
const static uint32_t MAX_OBJECTS_STORED_MEMORY = 16 * 1024 * 1024;
#endif

#if ICON7_USE_THREAD_CACHED_POOL
struct ListElement {
	ListElement *next;
	ListElement *n2;
	uint32_t count;
};

using PoolsQueue =
	moodycamel::ConcurrentQueue<ListElement *, ConcurrentQueueDefaultTraits>;

static PoolsQueue pools[NUMBER_OF_POOLS] = {
	PoolsQueue(1024 * 16), PoolsQueue(1024 * 16), PoolsQueue(1024 * 16),
	PoolsQueue(1024 * 16), PoolsQueue(1024 * 16), PoolsQueue(1024 * 16),
	PoolsQueue(1024 * 16), PoolsQueue(1024 * 16)};

struct PoolDestrctor {
	~PoolDestrctor()
	{
		for (int _i = 0; _i < 16; ++_i) {
			for (int i = 0; i < NUMBER_OF_POOLS; ++i) {
				ListElement *e = nullptr;
				while (pools[i].try_dequeue(e)) {
					while (e) {
						auto next = e->next;
						free(e);
						e = next;
					}
				}
			}
		}
		ICON7_MALLOC_TRIM();
	}
} _staticPoolsDestruction;

const static uint32_t objectCounts[NUMBER_OF_POOLS] = {1024, 1024, 1024, 1024,
													   1024, 1024, 1024, 64};

struct ObjectPoolTLS {
	PoolsQueue *queue;

	moodycamel::ConsumerToken ctok;
	moodycamel::ProducerToken ptok;

	ListElement *first = nullptr;
	ListElement *second = nullptr;
	int32_t firstCount = 0;
	int32_t secondCount = 0;

	const uint32_t POOL_ID;
	const uint32_t BYTES;
	const uint32_t MAX_OBJECTS;

	ObjectPoolTLS(uint32_t poolId)
		: queue(&(pools[poolId])), ctok(pools[poolId]), ptok(pools[poolId]),
		  POOL_ID(poolId), BYTES(objectSizes[poolId]),
		  MAX_OBJECTS(objectCounts[poolId])
	{
	}

	~ObjectPoolTLS()
	{
		if (first != nullptr) {
			queue->enqueue(ptok, first);
			first = nullptr;
		}
		if (second != nullptr) {
			queue->enqueue(ptok, second);
			second = nullptr;
		}
	}

	void ReleaseObject(void *ptr)
	{
		if (firstCount < MAX_OBJECTS) {
			ListElement *next = (ListElement *)ptr;
			firstCount++;
			next->next = first;
			first = next;
			return;
		} else if (secondCount > 0) {
			first->count = firstCount;
			queue->enqueue(ptok, first);

			first = (ListElement *)ptr;
			firstCount = 1;
			first->next = nullptr;
			return;
		} else {
			second = first;
			secondCount = firstCount;

			first = (ListElement *)ptr;
			firstCount = 1;
			first->next = nullptr;
			return;
		}
	}

	void *AcquireObject()
	{
		if (firstCount == 0) {
			if (secondCount > 0) {
				std::swap(first, second);
				std::swap(firstCount, secondCount);
			} else if (queue->try_dequeue(ctok, first) == true) {
				firstCount = first->count;
			} else {
				first = nullptr;
				return malloc(BYTES);
			}
		}
		ListElement *ret = first;
		first = first->next;
		firstCount--;
		return ret;
	}
};

struct PoolsTLS {
	ObjectPoolTLS pools[NUMBER_OF_POOLS];

	PoolsTLS()
		: pools(ObjectPoolTLS(0), ObjectPoolTLS(1), ObjectPoolTLS(2),
				ObjectPoolTLS(3), ObjectPoolTLS(4), ObjectPoolTLS(5),
				ObjectPoolTLS(6), ObjectPoolTLS(7))
	{
	}
};

thread_local PoolsTLS objectPoolTLS;
#else
using SizedPool =
	moodycamel::ConcurrentQueue<void *, ConcurrentQueueDefaultTraits>;

static SizedPool sizedQueues[NUMBER_OF_POOLS] = {
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[0]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[1]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[2]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[3]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[4]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[5]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[6]),
	SizedPool(MAX_OBJECTS_STORED_MEMORY / objectSizes[7]),
};

struct PoolDestrctor {
	~PoolDestrctor()
	{
		for (int _i = 0; _i < 3; ++_i) {
			for (int i = 0; i < NUMBER_OF_POOLS; ++i) {
				while (true) {
					void *ptr;
					if (sizedQueues[i].try_dequeue(ptr) == true) {
#if ICON7_MEMORY_COLLECT_STATS
						freedObjects++;
						freedMemory += objectSizes[i];
#endif
						free(ptr);
					} else {
						break;
					}
				}
			}
		}
		ICON7_MALLOC_TRIM();
	}
} _staticPoolsDestruction;
#endif
#endif

AllocatedObject<void> MemoryPool::Allocate(uint32_t bytes)
{
#if ICON7_USE_RPMALLOC
#if ICON7_MEMORY_COLLECT_STATS
	acquiredObjects++;
	allocatedObjects++;
	acquiredMemory += bytes;
	allocatedMemory += bytes;
#endif
	static int ____staticInitilizeRpmalloc = (rpmalloc_initialize(), 0);
	class __RpMallocThreadLocalDestructor final
	{
	public:
		__RpMallocThreadLocalDestructor() { rpmalloc_thread_initialize(); }
		~__RpMallocThreadLocalDestructor() { rpmalloc_thread_finalize(1); }
	};
	thread_local __RpMallocThreadLocalDestructor
		____staticThreadLocalDestructorRpmalloc;
	if (bytes >= 256) {
		return {rpaligned_alloc(64, bytes), bytes};
	} else {
		return {rpmalloc(bytes), bytes};
	}
#else
	if (bytes > BIGGEST_OBJECT_SIZE) {
#if ICON7_MEMORY_COLLECT_STATS
		acquiredObjects++;
		allocatedObjects++;
		acquiredMemory += bytes;
		allocatedMemory += bytes;
#endif
		return {malloc(bytes), bytes};
	}

	uint32_t poolId = GetPoolId(&bytes);

#if ICON7_MEMORY_COLLECT_STATS
	acquiredObjects++;
	acquiredMemory += bytes;
#endif

#if ICON7_USE_THREAD_CACHED_POOL
	PoolsTLS &tls = objectPoolTLS;
	return {tls.pools[poolId].AcquireObject(), bytes};
#else
	void *ptr = nullptr;
	if (sizedQueues[poolId].try_dequeue(ptr) == true) {
		return {ptr, bytes};
	}
#if ICON7_MEMORY_COLLECT_STATS
	allocatedObjects++;
	allocatedMemory += bytes;
#endif
	return {malloc(bytes), bytes};
#endif
#endif
}

void MemoryPool::Release(void *ptr, uint32_t bytes)
{
#if ICON7_USE_RPMALLOC
#if ICON7_MEMORY_COLLECT_STATS
	releasedObjects++;
	freedObjects++;
	releasedMemory += bytes;
	freedMemory += bytes;
#endif
	rpfree(ptr);
	return;
#else
	if (bytes > BIGGEST_OBJECT_SIZE) {
#if ICON7_MEMORY_COLLECT_STATS
		releasedObjects++;
		freedObjects++;
		releasedMemory += bytes;
		freedMemory += bytes;
#endif
		free(ptr);
		ICON7_MALLOC_TRIM();
		return;
	}

	uint32_t poolId = GetPoolId(&bytes);

#if ICON7_MEMORY_COLLECT_STATS
	releasedObjects++;
	releasedMemory += bytes;
#endif

#if ICON7_USE_THREAD_CACHED_POOL
	PoolsTLS &tls = objectPoolTLS;
	tls.pools[poolId].ReleaseObject(ptr);
#else
	if (sizedQueues[poolId].size_approx() >
		(MAX_OBJECTS_STORED_MEMORY / objectSizes[poolId])) {
#if ICON7_MEMORY_COLLECT_STATS
		freedObjects++;
		freedMemory += bytes;
#endif
		free(ptr);
		ICON7_MALLOC_TRIM();
		return;
	} else if (sizedQueues[poolId].enqueue(ptr) == false) {
#if ICON7_MEMORY_COLLECT_STATS
		freedObjects++;
		freedMemory += bytes;
#endif
		free(ptr);
		ICON7_MALLOC_TRIM();
		return;
	}
// 		if (sizedQueues[poolId].enqueue(ptr) == false) {
// 			printf("f");
// 			free(ptr);
// 			ICON7_MALLOC_TRIM();
// 			return;
// 		}
#endif
#endif
}

#if ICON7_USE_RPMALLOC
#else
uint32_t MemoryPool::RoundSize(uint32_t bytes) { return std::bit_ceil(bytes); }

uint32_t MemoryPool::GetBits(uint32_t bytes) { return std::bit_width(bytes); }

uint32_t MemoryPool::GetPoolId(uint32_t *bytes)
{
	if (*bytes < SMALLES_OBJECT_SIZE) {
		*bytes = SMALLES_OBJECT_SIZE;
	}
	*bytes = RoundSize(*bytes);
	int pool = GetBits(*bytes) - std::bit_width(SMALLES_OBJECT_SIZE);
	return pool;
}
#endif

} // namespace icon7
