// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdint>

#include <random>
#include <mutex>

namespace icon7
{
static uint64_t InitRandom()
{
	static std::mutex mutex;
	mutex.lock();
	static std::random_device rd{};
	static std::mt19937_64 mt{rd()};
	uint64_t v = mt();
	mutex.unlock();
	return v;
}

uint64_t RandHashBase()
{
	/*thread_local */ static std::mt19937_64 mt(InitRandom());
	return mt();
}
} // namespace icon7
