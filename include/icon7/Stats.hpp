// Copyright (C) 2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_STATS_HPP
#define ICON7_STATS_HPP

#include <cstdint>

#include <atomic>

#include "Time.hpp"

namespace icon7
{
struct PeerStats {
	PeerStats();

	volatile int64_t bytesSent = 0;
	volatile int64_t bytesReceived = 0;

	volatile int64_t packetsSent = 0;
	volatile int64_t packetsReceived = 0;

	volatile int64_t framesSent = 0;
	volatile int64_t framesReceived = 0;

	volatile int64_t onWriteable = 0;

	volatile int64_t errorsCount = 0;

	volatile time::Timestamp startTimestamp = {0};
};

struct HostStats : public PeerStats {
	volatile int64_t timeouts = 0;
	volatile int64_t longTimeouts = 0;

	volatile int64_t connectionsRemoteTotal = 0;
	volatile int64_t connectionsRemoteSuccessfull = 0;

	volatile int64_t connectionsLocalTotal = 0;
	volatile int64_t connectionsLocalSuccessfull = 0;

	volatile int64_t connectionsFailed = 0;

	volatile int64_t disconnectedTotal = 0;
	volatile int64_t disconnectedLocal = 0;
	volatile int64_t disconnectedRemote = 0;
};

struct LoopStats : public HostStats {
	volatile int64_t loopWakeups = 0;
	volatile int64_t loopBigIterations = 0;
	volatile int64_t loopTimerWakeups = 0;
	volatile int64_t loopSmallIterations = 0;
};

struct MemoryStats {
	alignas(64) std::atomic<int64_t> smallAllocations = 0;
	alignas(64) std::atomic<int64_t> mediumAllocations = 0;
	alignas(64) std::atomic<int64_t> largeAllocations = 0;
	alignas(64) std::atomic<int64_t> allocatedBytes = 0;
#if ICON7_USE_RPMALLOC
	alignas(64) std::atomic<int64_t> maxInUseAtOnce = 0;
	alignas(64) std::atomic<int64_t> smallDeallocations = 0;
	alignas(64) std::atomic<int64_t> mediumDeallocations = 0;
	alignas(64) std::atomic<int64_t> largeDeallocations = 0;
	alignas(64) std::atomic<int64_t> deallocatedBytes = 0;
#else
	alignas(64) std::atomic<int64_t> deallocations = 0;
#endif

	volatile time::Timestamp startTimestamp = {0};

	MemoryStats();

	inline const static size_t MAX_BYTES_FOR_SMALL_ALLOCATIONS = 256;
	inline const static size_t MAX_BYTES_FOR_MEDIUM_ALLOCATIONS = 4096;
};
} // namespace icon7

#endif
