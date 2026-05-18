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
			data[li.id].loop = std::move(value);
		}
		return data[li.id].Set(std::move(loop));
	}

	T &Set(Loop *loop, const T &value)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			data.resize(li.id+1);
			data[li.id].loop = loop;
		}
		return data[li.id] = value;
	}

	T &Access(Loop *loop)
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			data.resize(li.id+1);
			data[li.id].loop = loop;
		}
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
		return data[li.id].Get(loop);
	}

	const T *Get(Loop *loop) const
	{
		assert(loop);
		LoopIndex li = loop->loopIndex.index;
		if (data.size() <= li.id) {
			[[unlikely]];
			return nullptr;
		}
		return data[li.id].Get(loop);
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
};

}

#endif
