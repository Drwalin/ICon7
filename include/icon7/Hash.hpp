// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_HASH_HPP
#define ICON7_HASH_HPP

#include <cstdint>
#include <cstring>

#include <string_view>
#include <string>
#include <memory>

// This hash value is not cross-platform inter-operatable

namespace icon7
{
constexpr static bool ICON7_USE_FNV_MULT = false;

constexpr uint32_t StartHash32()
{
	constexpr uint32_t base = 0x811c9dc5;
	return base;
}
constexpr uint32_t Hash32(const uint32_t a, const uint32_t b)
{
	constexpr uint32_t prime = 0x01000193;
	const uint32_t h = (b ^ a);
	
	if constexpr (ICON7_USE_FNV_MULT) {
		return h * prime;
	} else {
		const uint32_t a = h + (h << 1);
		const uint32_t b = (h << 24) + (h << 4);
		return b + a + (a << 7);
	}
}

constexpr uint64_t StartHash64()
{
	constexpr uint64_t base = 0xcbf29ce484222325llu;
	return base;
}
constexpr uint64_t Hash64(const uint64_t a, const uint64_t b)
{
	constexpr uint64_t prime = 0x00000100000001b3llu;
	const uint64_t h = (b ^ a);
	
	if constexpr (ICON7_USE_FNV_MULT) {
		return h * prime;
	} else {
		const uint64_t a = h + (h << 1);
		const uint64_t b = h << 40;
		return b + a + (a << 4) + (a << 7);
	}
}

template<typename T>
struct Hash {
	static constexpr uint32_t hash(const T k, uint64_t base);
};

template<>
struct Hash <uint64_t> {
	static constexpr uint32_t hash(const uint64_t k, uint64_t base = StartHash64())
	{
		return Hash64(k, base);
	}
};

template<>
struct Hash <uint32_t> {
	static constexpr uint32_t hash(const uint32_t k, uint64_t base = StartHash32())
	{
		return Hash32(k, base);
	}
};

template<>
struct Hash <int32_t> {
	static constexpr uint32_t hash(const int32_t k, uint64_t base = StartHash32())
	{
		return Hash32((uint32_t)k, base);
	}
};

template<>
struct Hash <int64_t> {
	static constexpr uint32_t hash(const int64_t k, uint64_t base = StartHash32())
	{
		return Hash32((uint64_t)k, base);
	}
};

template<typename T>
struct Hash <T *> {
	uint32_t hash(const T *k, uint64_t base = sizeof(T*)==sizeof(uint64_t) ? StartHash64() : StartHash32())
	{
		if constexpr (sizeof(T*) == sizeof(uint64_t)) {
			return Hash<uint64_t>::hash((uint64_t)k, base);
		} else if constexpr (sizeof(T*) == sizeof(uint32_t)) {
			return Hash<uint32_t>::hash((uint32_t)k, base);
		} else {
			static_assert(!"sizeof(T*) needs to be 4 or 8 bytes, but is not.");
		}
	}
};

template<typename T>
struct Hash <std::shared_ptr<T>> {
	uint32_t hash(const std::shared_ptr<T> &k, uint64_t base = StartHash64())
	{
		return Hash<T*>(k.get(), base);
	}
};

template<>
struct Hash <std::string_view> {
	static constexpr uint32_t hash(const std::string_view k, uint64_t base = StartHash64())
	{
		const char *ptr = k.data();
		const uint32_t size = k.size();
		uint32_t i = 0;
		uint64_t h = base;
		for (; i+8 < size; i += 8) {
			uint64_t v = *(const uint64_t*)(ptr+i);
			h = Hash64(h, v);
		}
		uint64_t v = 0;
		memcpy(&v, ptr+i, size - i);
		return Hash64(h, v);
	}
};

template<>
struct Hash <std::string> {
	static constexpr uint32_t hash(const std::string &k, uint64_t base = StartHash64())
	{
		return Hash<std::string_view>::hash(std::string_view(k), base);
	}
};
}
#endif
