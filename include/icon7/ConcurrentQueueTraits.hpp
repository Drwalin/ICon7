// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_CONCURRENT_QUEUE_TRAITS_HPP
#define ICON7_CONCURRENT_QUEUE_TRAITS_HPP

#include "../../concurrentqueue/concurrentqueue.h"

namespace icon7
{
struct ConcurrentQueueDefaultTraits
	: public moodycamel::ConcurrentQueueDefaultTraits {
	static const int MAX_SEMA_SPINS = 100;
	static const bool RECYCLE_ALLOCATED_BLOCKS = false;
};
} // namespace icon7

#endif
