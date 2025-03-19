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

#ifndef ICON7_COROUTINE_HELPER_HPP
#define ICON7_COROUTINE_HELPER_HPP

#include <coroutine>

namespace icon7
{
struct CoroutineSchedulable {
	struct promise_type {
		CoroutineSchedulable get_return_object() { return {}; }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() {}

		void *operator new(std::size_t bytes);
		void operator delete(void *ptr, std::size_t bytes);
	};
};
} // namespace icon7

#endif
