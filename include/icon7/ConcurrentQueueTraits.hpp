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
