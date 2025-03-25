// Copyright (C) 2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <new>

#include <cstdio>

#include "../include/icon7/MemoryPool.hpp"

extern void operator delete(void *p) noexcept
{
	icon7::MemoryPool::Release(p, 0);
}

extern void operator delete[](void *p) noexcept
{
	icon7::MemoryPool::Release(p, 0);
}

extern void *operator new(std::size_t size) noexcept(false)
{
	return icon7::MemoryPool::Allocate(size).object;
}

extern void *operator new[](std::size_t size) noexcept(false)
{
	return icon7::MemoryPool::Allocate(size).object;
}

extern void *operator new(std::size_t size, const std::nothrow_t &tag) noexcept
{
	(void)sizeof(tag);
	return icon7::MemoryPool::Allocate(size).object;
}

extern void *operator new[](std::size_t size,
							const std::nothrow_t &tag) noexcept
{
	(void)sizeof(tag);
	return icon7::MemoryPool::Allocate(size).object;
}

extern void operator delete(void *p, std::size_t size) noexcept
{
	(void)sizeof(size);
	icon7::MemoryPool::Release(p, 0);
}

extern void operator delete[](void *p, std::size_t size) noexcept
{
	(void)sizeof(size);
	icon7::MemoryPool::Release(p, 0);
}
