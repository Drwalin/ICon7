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

template <typename K, typename V, typename THash = Hash<K>>
class FlatCuckooHashMap;

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
		const uint32_t bucketHead = Bucket(key);
		PairIds elemIds = GetElement(key, bucketHead);
		if (elemIds.current == 0) {
			return _Insert(std::move(key), std::move(value), bucketHead);
		}

		Pair *elem = &(buckets[elemIds.current - 1]);
		if constexpr (requires { elem->v = std::move(value); }) {
			elem->v = std::move(value);
		} else {
			elem->v.~V();
			new (&(elem->v)) V(std::move(value));
		}
		return elem->v;
	}

	V &_Insert(K &&key, V &&value, uint32_t bucketHead)
	{
		if (heads.size() < (buckets.size() + (buckets.size() >> 1))) {
			Rehash(heads.size() << 1);
			bucketHead = Bucket(key);
		}

		Pair &p = buckets.emplace_back(std::move(key), std::move(value),
									   heads[bucketHead]);
		const uint32_t current = buckets.size();
		heads[bucketHead] = current;
		return p.v;
	}

	const V *Get(const K &key) const
	{
		const uint32_t bucketHead = Bucket(key);
		PairIds elemIds = GetElement(key, bucketHead);
		if (elemIds.current == 0) {
			return nullptr;
		}
		return &(buckets[elemIds.current - 1].v);
	}

	V *Get(const K &key)
	{
		return (V *)(((const FlatHashMap *)this)->Get(key));
	}

	bool Has(const K &key) const
	{
		const V *v = Get(key);
		if (v == nullptr) {
			return false;
		}
		return true;
	}

	void Remove(const K &key)
	{
		if (buckets.size() == 0) {
			return;
		}

		if (buckets.size() == 1) {
			if (buckets[0].key == key) {
				const uint32_t bucketHead = Bucket(key);
				assert(heads[bucketHead] == 1);
				heads[bucketHead] = 0;
				buckets.clear();
			}
			return;
		}

		const uint32_t bucketHeadRem = Bucket(key);
		PairIds pairRem = GetElement(key, bucketHeadRem);

		if (pairRem.current == 0) {
			// Key does not exists
			return;
		}

		{
			// Removing current key from bucket chain
			const uint32_t next = buckets[pairRem.current - 1].next;
			if (pairRem.prev == 0) {
				heads[bucketHeadRem] = next;
			} else {
				buckets[pairRem.prev - 1].next = next;
			}
		}

		if (pairRem.current == buckets.size()) {
			// Key is last in buckets, shrink buckets by 1 and return
			buckets.resize(buckets.size() - 1);
			return;
		}

		// Determining how to move last key into place of current in buckets
		const K &keyLast = buckets.back().key;
		const uint32_t bucketHeadLast = Bucket(keyLast);
		PairIds pairLast = GetElement(keyLast, bucketHeadLast);

		if (pairLast.prev == pairRem.current) {
			pairLast.prev = pairRem.prev;
		}

		// Move last to place of current
		if constexpr (requires { buckets[0] = std::move(buckets[1]); }) {
			buckets[pairRem.current - 1] = std::move(buckets.back());
		} else {
			buckets[pairRem.current - 1].~Pair();
			new (&(buckets[pairRem.current - 1]))
				Pair(std::move(buckets.back()));
		}

		// Destroy last empty (moved
		buckets.resize(buckets.size() - 1);

		// Update position of last element which was moved to place of
		// removed element
		if (pairLast.prev == 0) {
			heads[bucketHeadLast] = pairRem.current;
		} else {
			buckets[pairLast.prev - 1].next = pairRem.current;
		}
	}

	void Rehash(uint32_t newCapacity)
	{
		const uint32_t capacity = RoundCapacity(newCapacity, buckets.size());

		const uint32_t oldSize = heads.size();
		heads.resize(capacity);
		for (uint32_t i = 0; i < oldSize; ++i) {
			heads[i] = 0;
		}

		std::vector<Pair> oldBuckets;
		std::swap(oldBuckets, buckets);

		for (Pair &p : oldBuckets) {
			K &key = p.key;
			uint32_t bucketHead = Bucket(key);
			_Insert(std::move(key), std::move(p.v), bucketHead);
		}
	}

	void Clear()
	{
		if (buckets.size() == 0) {
			return;
		}
		buckets.clear();
		for (uint32_t &v : heads) {
			v = 0;
		}
	}

	uint32_t Size() const { return buckets.size(); }

	uint32_t Capacity() const { return heads.size(); }

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
		return &(buckets[(uint32_t)RandHashBase() % buckets.size()]);
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
		assert((heads.size() & (heads.size() - 1)) == 0);
		return THash::hash(key, baseHash) & ((uint32_t)heads.size() - 1);
	}

	struct PairIds {
		uint32_t current, prev;
	};

	PairIds GetElement(const K &key, uint32_t bucketHead) const
	{
		uint32_t prev = 0;
		assert(Bucket(key) == bucketHead);
		const uint32_t bucket = heads[bucketHead];
		for (uint32_t i = bucket; i != 0;) {
			const Pair &bucket = buckets[i - 1];
			if (bucket.key == key) {
				return {i, prev};
			}
			prev = i;
			i = bucket.next;
		}
		return {0, 0};
	}

private:
	std::vector<uint32_t> heads;
	std::vector<Pair> buckets;
	const uint64_t baseHash = RandHashBase();
};
} // namespace icon7
#endif
