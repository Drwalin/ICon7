// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FLAT_HASH_MAP_HPP
#define ICON7_FLAT_HASH_MAP_HPP

#include "FlatHashMap.hpp"

namespace icon7
{
/*
template <typename K, typename V, typename THash> class FlatCuckooHashMap
{
public:
	FlatCuckooHashMap(uint32_t initialCapacity)
	{
		initialCapacity = RoundCapacity(initialCapacity, 16);
		tables[0].Resize(initialCapacity, {});
		tables[1].Resize(initialCapacity, {});
		existing.resize(initialCapacity, 0);
		base[0] = RandHashBase();
		base[1] = RandHashBase();
	}
	FlatCuckooHashMap() : FlatCuckooHashMap(16) {}

	FlatCuckooHashMap(FlatCuckooHashMap &o) = default;
	FlatCuckooHashMap(const FlatCuckooHashMap &o) = default;
	FlatCuckooHashMap(FlatCuckooHashMap &&o) = default;

	FlatCuckooHashMap &operator=(FlatCuckooHashMap &o) = default;
	FlatCuckooHashMap &operator=(const FlatCuckooHashMap &o) = default;
	FlatCuckooHashMap &operator=(FlatCuckooHashMap &&o) = default;

	void InsertOrSet(K &key, V &val)
	{
		K k = key;
		V v = val;
		return InsertOrSet(std::move(k), std::move(v));
	}
	void InsertOrSet(K &&key, V &val)
	{
		V v = val;
		return InsertOrSet(std::move(key), std::move(v));
	}
	void InsertOrSet(K &key, V &&val)
	{
		K k = key;
		return InsertOrSet(std::move(k), std::move(val));
	}
	V &InsertOrSet(K &&key, V &&value)
	{
	}

	V *Get(const K &key)
	{

	}

	const V *Get(const K &key) const
	{
	}

	void Remove(const K &key)
	{
	}

	void Rehash(uint32_t newCapacity)
	{
	}

	void Clear()
	{
		overflow.Clear();
		for (int i=0; i<existing.size(); ++i) {
			uint8_t e = existing[i];
			if (e & 1) {
				tables[i][0].existing.~ExistingPair();
			}
			if (e & 2) {
				tables[i][1].existing.~ExistingPair();
			}
		}
		memset(existing.data(), 0, existing.size());
		size = 0;
	}

private:
	struct IntsPair {
		uint64_t a, b;
	};

	struct ExistingPair {
		K key;
		V value;
	};
	struct Pair {
		union {
			uint8_t _;
			ExistingPair existing;
		};
	};

private:
	IntsPair Bucket(const K &key) {
		return {
			THash::hash(key, base[0]),
			THash::hash(key, base[1])
		};
	}

private:
	std::vector<uint8_t> existing;
	std::vector<Pair[2]> tables;
	FlatHashMap<K, V, THash> overflow;
	uint64_t base[2];
	uint32_t size = 0;
};
*/
} // namespace icon7
#endif
