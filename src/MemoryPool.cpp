// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Debug.hpp"

#if !defined(ICON7_USE_RPMALLOC)
#define ICON7_USE_RPMALLOC 0
#endif

#if ICON7_USE_RPMALLOC
#include "../rpmalloc/rpmalloc/rpmalloc.h"
#else
#include <cstdlib>
#endif

#include "../include/icon7/MemoryPool.hpp"

namespace icon7
{
void MemoryPool::PrintStats()
{
#if ICON7_USE_RPMALLOC
	LOG_WARN("Printing memory stats not implemented.");
#else
	LOG_WARN("Printing memory stats not implemented.");
#endif
}

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
	return {rpmalloc(bytes), bytes};
#else
	return {malloc(bytes), bytes};
#endif
}

void MemoryPool::Release(void *ptr, size_t bytes)
{
#if ICON7_USE_RPMALLOC
	rpfree(ptr);
	return;
#else
	free(ptr);
#endif
}
} // namespace icon7
