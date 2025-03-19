/*
 *  This file is part of ICon7.
 *  Copyright (C) 2025 Marek Zalewski aka Drwalin
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

#include <new>

#include <cstdio>

#include "../include/icon7/MemoryPool.hpp"

extern void 
operator delete(void* p) noexcept {
	icon7::MemoryPool::Release(p, 0);
}

extern void 
operator delete[](void* p) noexcept {
	icon7::MemoryPool::Release(p, 0);
}

extern void* 
operator new(std::size_t size) noexcept(false) {
	return icon7::MemoryPool::Allocate(size).object;
}

extern void* 
operator new[](std::size_t size) noexcept(false) {
	return icon7::MemoryPool::Allocate(size).object;
}

extern void* 
operator new(std::size_t size, const std::nothrow_t& tag) noexcept {
	(void)sizeof(tag);
	return icon7::MemoryPool::Allocate(size).object;
}

extern void* 
operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
	(void)sizeof(tag);
	return icon7::MemoryPool::Allocate(size).object;
}

extern void 
operator delete(void* p, std::size_t size) noexcept {
	(void)sizeof(size);
	icon7::MemoryPool::Release(p, 0);
}

extern void 
operator delete[](void* p, std::size_t size) noexcept {
	(void)sizeof(size);
	icon7::MemoryPool::Release(p, 0);
}
