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

#include <cstdio>

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
	printf("Nope");
#else
	printf("Memory stats not collected\n");
	fflush(stdout);
#endif
}

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
#endif

AllocatedObject<void> MemoryPool::Allocate(uint32_t bytes)
{
#if ICON7_USE_RPMALLOC
	return {rpmalloc(bytes), bytes};
#else
	return {malloc(bytes), bytes};
#endif
}

void MemoryPool::Release(void *ptr, uint32_t bytes)
{
#if ICON7_USE_RPMALLOC
	rpfree(ptr);
	return;
#else
	free(ptr);
#endif
}
} // namespace icon7
