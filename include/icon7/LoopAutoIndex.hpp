// Copyright (C) 2025-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_LOOP_AUTO_INDEX_HPP
#define ICON7_LOOP_AUTO_INDEX_HPP

#include <memory>

namespace icon7
{
class Loop;

struct LoopIndex
{
	uint32_t version = 0;
	uint32_t id = 0;

	std::shared_ptr<Loop> GetLoop() const;
};

class LoopAutoIndex
{
public:
	LoopAutoIndex(Loop *loop);
	~LoopAutoIndex();

	LoopIndex index;
	
	inline operator LoopIndex() const { return index; }

private:
	LoopAutoIndex() = delete;
	LoopAutoIndex(LoopAutoIndex &) = delete;
	LoopAutoIndex(LoopAutoIndex &&) = delete;
	LoopAutoIndex(const LoopAutoIndex &) = delete;
	LoopAutoIndex &operator=(LoopAutoIndex &) = delete;
	LoopAutoIndex &operator=(LoopAutoIndex &&) = delete;
	LoopAutoIndex &operator=(const LoopAutoIndex &) = delete;
};
}

#endif
