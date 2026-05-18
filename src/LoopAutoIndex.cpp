// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <shared_mutex>
#include <vector>
#include <memory>

#include "../include/icon7/Host.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Loop.hpp"

#include "../include/icon7/LoopAutoIndex.hpp"

namespace icon7
{
struct LoopAutoIndexImpl
{
	LoopAutoIndexImpl()
	{
		data.resize(1);
	}

	LoopIndex NewId(Loop *loop)
	{
		assert(loop);
		std::lock_guard lock(mutex);
		for (uint32_t i=1; i<data.size(); ++i) {
			if (data[i].loop == nullptr) {
				LoopIndex ret;
				ret.id = i;
				ret.version = data[i].version;
				data[i].loop = loop;
				
			}
		}
		LoopIndex ret;
		ret.id = data.size();
		ret.version = 0;
		data.push_back({loop, 0});
		return ret;
	}

	void ReleaseId(LoopIndex index)
	{
		std::lock_guard lock(mutex);
		if (index.id >= data.size()) {
			LOG_FATAL("Invalid loop index id.");
			return;
		}
		if (index.id == 0) {
			LOG_FATAL("Invalid loop index id.");
			return;
		}
		if (data[index.id].version != index.version) {
			LOG_WARN("Trying to release old loop index version.");
			return;
		}
		if (data[index.id].loop == nullptr) {
			LOG_FATAL("Trying to release already released loop index.");
			return;
		}
		data[index.id].loop = nullptr;
		data[index.id].version++;
	}

	std::shared_ptr<Loop> GetLoop(LoopIndex index)
	{
		std::shared_lock lock(mutex);
		if (index.id >= data.size()) {
			LOG_FATAL("Invalid loop index id");
			return nullptr;
		}
		if (index.id == 0) {
			LOG_FATAL("Invalid loop index id");
			return nullptr;
		}
		if (data[index.id].version != index.version) {
			LOG_WARN("Invalid loop index version");
			return nullptr;
		}
		if (data[index.id].loop == nullptr) {
			return nullptr;
		}
		return data[index.id].loop->shared_from_this();
	}

public:
	struct Data
	{
		Loop *loop = nullptr;
		uint32_t version = 0;
	};

	std::vector<Data> data;
	std::shared_mutex mutex;
};

static LoopAutoIndexImpl loopAutoIndexImpl;

LoopAutoIndex::LoopAutoIndex(Loop *loop)
{
	index = loopAutoIndexImpl.NewId(loop);
}

LoopAutoIndex::~LoopAutoIndex()
{
	loopAutoIndexImpl.ReleaseId(index);
	index = {0,0};
}

std::shared_ptr<Loop> LoopIndex::GetLoop() const
{
	return loopAutoIndexImpl.GetLoop(*this);
}
} // namespace icon7
