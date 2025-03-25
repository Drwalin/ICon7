// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
