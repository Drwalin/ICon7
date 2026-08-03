// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_MAP_LOOP_HPP
#define ICON7_MAP_LOOP_HPP

#include <vector>

#include "Loop.hpp"
#include "PeerHandle.hpp"

namespace icon7
{
template<typename T>
struct MapLoop
{
	T &Set(Loop *loop, T &&value)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			data.resize(li.id+1);
			loops.resize(data.size(), nullptr);
		}
		if (loops[li.id] == nullptr) {
			loops[li.id] = loop;
		}
		assert(loops[li.id] == loop);
		data[li.id] = std::move(value);
		return data[li.id];
	}

	T &Set(Loop *loop, const T &value)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			data.resize(li.id+1);
			loops.resize(data.size(), nullptr);
		}
		if (loops[li.id] == nullptr) {
			loops[li.id] = loop;
		}
		assert(loops[li.id] == loop);
		data[li.id] = value;
		return data[li.id];
	}

	T &Access(Loop *loop)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			data.resize(li.id+1);
			loops.resize(data.size(), nullptr);
		}
		if (loops[li.id] == nullptr) {
			loops[li.id] = loop;
		}
		assert(loops[li.id] == loop);
		return data[li.id];
	}

	T *Get(Loop *loop)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			return nullptr;
		}
		if (loops[li.id] == nullptr) {
			return nullptr;
		}
		assert(loops[li.id] == loop);
		return &(data[li.id]);
	}

	const T *Get(Loop *loop) const
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			return nullptr;
		}
		if (loops[li.id] == nullptr) {
			return nullptr;
		}
		assert(loops[li.id] == loop);
		return &(data[li.id]);
	}

	void Remove(Loop *loop)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
		}
		if (loops[li.id] != nullptr) {
			assert(loops[li.id] == loop);
			loops[li.id] = loop;
			data[li.id] = {};
		}
	}

	T &operator[](Loop *loop)
	{
		return Access(loop);
	}

	uint32_t Size() const
	{
		return data.size();
	}

public:
	std::vector<T> data;
	std::vector<Loop *> loops;
};

}

#endif
