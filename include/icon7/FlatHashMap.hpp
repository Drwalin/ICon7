// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FLAT_HASH_MAP_HPP
#define ICON7_FLAT_HASH_MAP_HPP

#include <cassert>
#include <cstdint>

#include <vector>

#include "Hash.hpp"

namespace icon7
{
uint64_t RandHashBase();

template <typename K, typename V, typename THash = Hash<K>> class FlatCuckooHashMap;

constexpr uint32_t RoundCapacity(uint32_t newCapacity, uint32_t min)
{
	uint32_t capacity = 16;
	uint32_t minCap = min + (min >> 1) + 5;
	if (newCapacity < minCap) {
		newCapacity = minCap;
	}
	while (capacity < newCapacity) {
		capacity <<= 1;
	}
	return capacity;
}

template <typename K, typename V, typename THash = Hash<K>> class FlatHashMap
{
public:
	FlatHashMap(uint32_t initialCapacity)
	{
		if (initialCapacity < 16) {
			initialCapacity = 16;
		}
		assert(((initialCapacity & (initialCapacity - 1)) == 0) &&
			   "FlatHashMap initialCapacity needs to be power of 2");
		heads.resize(initialCapacity, 0);
	}
	FlatHashMap() : FlatHashMap(16) {}

	FlatHashMap(FlatHashMap &o) = default;
	FlatHashMap(const FlatHashMap &o) = default;
	FlatHashMap(FlatHashMap &&o) = default;

	FlatHashMap &operator=(FlatHashMap &o) = default;
	FlatHashMap &operator=(const FlatHashMap &o) = default;
	FlatHashMap &operator=(FlatHashMap &&o) = default;

	V &InsertOrSet(const K &key, V &val)
	{
		K k = key;
		V v = val;
		return InsertOrSet(std::move(k), std::move(v));
	}
	V &InsertOrSet(K &key, const V &val)
	{
		K k = key;
		V v = val;
		return InsertOrSet(std::move(k), std::move(v));
	}
	V &InsertOrSet(const K &key, const V &val)
	{
		K k = key;
		V v = val;
		return InsertOrSet(std::move(k), std::move(v));
	}
	V &InsertOrSet(K &key, V &val)
	{
		K k = key;
		V v = val;
		return InsertOrSet(std::move(k), std::move(v));
	}
	V &InsertOrSet(K &&key, V &val)
	{
		V v = val;
		return InsertOrSet(std::move(key), std::move(v));
	}
	V &InsertOrSet(K &key, V &&val)
	{
		K k = key;
		return InsertOrSet(std::move(k), std::move(val));
	}
	V &InsertOrSet(const K &key, V &&val)
	{
		K k = key;
		return InsertOrSet(std::move(k), std::move(val));
	}

	V &InsertOrSet(K &&key, V &&value)
	{
		uint32_t buck = Bucket(key);
		uint32_t bucket = heads[buck];
		for (uint32_t i = bucket; i != 0;) {
			Pair &bucket = buckets[i - 1];
			if (bucket.key == key) {
				if constexpr (requires {bucket.v = std::move(value);}) {
					bucket.v = std::move(value);
				} else {
					bucket.v.~V();
					new (&bucket.v) V(std::move(value));
				}
				return bucket.v;
			}
			i = bucket.next;
		}

		if (heads.size() < (buckets.size() + (buckets.size() >> 1))) {
			Rehash(heads.size() << 1);
			bucket = heads[buck];
		}

		Pair &p = buckets.emplace_back(std::move(key), std::move(value),
									   heads[bucket]);
		const uint32_t offset = buckets.size();
		heads[bucket] = offset;
		return p.v;
	}

	V *Get(const K &key)
	{
		const uint32_t bucket = heads[Bucket(key)];
		for (uint32_t i = bucket; i != 0;) {
			Pair &bucket = buckets[i - 1];
			if (bucket.key == key) {
				return &bucket.v;
			}
			i = bucket.next;
		}
		return nullptr;
	}

	const V *Get(const K &key) const
	{
		const uint32_t bucket = heads[Bucket(key)];
		for (uint32_t i = bucket; i != 0;) {
			const Pair &bucket = buckets[i - 1];
			if (bucket.key == key) {
				return &bucket.v;
			}
			i = bucket.next;
		}
		return nullptr;
	}
	
	bool Has(const K &key) const
	{
		return Get(key) != nullptr;
	}

	void Remove(const K &key)
	{
		const uint32_t bucket = Bucket(key);
		const int32_t prev = FindPrevId(bucket, key);
		if (prev < 0) {
			return;
		}
		const uint32_t current = GetNext(prev, bucket);

		Pair &b = buckets[current - 1];

		uint32_t next = b.next;
		if (prev) {
			buckets[prev - 1].next = next;
		} else {
			heads[bucket] = next;
		}

		if (current == buckets.size()) {
			buckets.resize(buckets.size() - 1);
			return;
		}

		Pair &dst = buckets[current - 1];
		dst.key.~K();
		dst.v.~V();

		Pair &last = buckets.back();
		new (&dst.key) K(std::move(last.key));
		new (&dst.v) V(std::move(last.v));
		dst.next = last.next;

		buckets.resize(buckets.size() - 1);

		const uint32_t otherBucket = Bucket(dst.key);
		const int32_t otherPrev = FindPrevId(otherBucket, dst.key);
		assert(otherPrev >= 0);

		if (otherPrev) {
			buckets[otherPrev - 1].next = current;
		} else {
			heads[otherBucket] = current;
		}
	}

	void Rehash(uint32_t newCapacity)
	{
		const uint32_t capacity = RoundCapacity(newCapacity, buckets.size());

		uint32_t minZero = heads.size();
		if (minZero > capacity) {
			minZero = capacity;
		}

		heads.resize(capacity, 0);
		for (uint32_t i = 0; i < minZero; ++i) {
			heads[i] = 0;
		}

		for (uint32_t i = 0; i < buckets.size(); ++i) {
			Pair &b = buckets[i];
			uint32_t bucket = heads[Bucket(b.key)];
			b.next = heads[bucket];
			heads[bucket] = i;
		}
	}

	void Clear() {
		buckets.clear();
		for (uint32_t &v : heads) {
			v = 0;
		}
	}

	uint32_t Size() const
	{
		return buckets.size();
	}

	uint32_t Capacity() const
	{
		return buckets.capacity();
	}

public:
	struct Pair {
		K key;
		V v;
		uint32_t next;
	};
	
	friend class FlatCuckooHashMap<K, V, THash>;
	
	Pair *GetRandom()
	{
		if (buckets.size() == 0) {
			return nullptr;
		}
		return &(buckets[(uint32_t )RandHashBase() % buckets.size()]);
	}

private:
	int32_t FindPrevId(uint32_t bucket, const K &key) const
	{
		int32_t prev = 0;
		for (uint32_t i = heads[bucket]; i != 0;) {
			const Pair &b = buckets[i - 1];
			if (b.key == key) {
				return prev;
			}
			prev = i;
			i = b.next;
		}
		return -1;
	}

	uint32_t GetNext(int32_t prev, uint32_t bucket) const
	{
		if (prev > 0) {
			return buckets[prev - 1].next;
		} else if (prev == 0) {
			return heads[bucket];
		} else {
			return 0;
		}
	}

	uint32_t Bucket(const K &key) const
	{
		return THash::hash(key, baseHash) & ((uint32_t)heads.size() - 1);
	}

private:
	std::vector<uint32_t> heads;
	std::vector<Pair> buckets;
	const uint64_t baseHash = RandHashBase();
};

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
