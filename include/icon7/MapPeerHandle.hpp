// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_MAP_PEER_HANDLE_HPP
#define ICON7_MAP_PEER_HANDLE_HPP

#include <vector>

#include "MapLoop.hpp"
#include "PeerHandle.hpp"

namespace icon7
{
template<typename T>
struct MapPeerHandleInLoop
{
	T &Set(PeerHandle handle, T &&value)
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		T *ptr = Get(handle);
		if (ptr != nullptr) {
			[[likely]];
			return *ptr;
		}
		if (entityDataOffsets.size() <= handle.id.id) {
			[[unlikely]];
			entityDataOffsets.resize(handle.id.id+1, 0);
		}
		const uint32_t offset = denseData.size();
		denseData.emplace_back(std::move(value));
		denseDataEntityIds.push_back(handle.id);
		entityDataOffsets[handle.id.id] = offset;
	}

	T &Set(PeerHandle handle, const T &value)
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		T *ptr = Get(handle);
		if (ptr != nullptr) {
			[[likely]];
			return *ptr;
		}
		if (entityDataOffsets.size() <= handle.id.id) {
			[[unlikely]];
			entityDataOffsets.resize(handle.id.id+1, 0);
		}
		const uint32_t offset = denseData.size();
		denseData.push_back(value);
		denseDataEntityIds.push_back(handle.id);
		entityDataOffsets[handle.id.id] = offset;
	}

	T &Access(PeerHandle handle)
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		T *ptr = Get(handle);
		if (ptr != nullptr) {
			[[likely]];
			return *ptr;
		}
		if (entityDataOffsets.size() <= handle.id.id) {
			[[unlikely]];
			entityDataOffsets.resize(handle.id.id+1, 0);
		}
		const uint32_t offset = denseData.size();
		denseData.resize(offset + 1);
		denseDataEntityIds.push_back(handle.id);
		entityDataOffsets[handle.id.id] = offset;
	}

	void Remove(PeerHandle handle)
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		if (entityDataOffsets.size() >= handle.id.id) {
			[[unlikely]];
			return;
		}
		const uint32_t offset = entityDataOffsets[handle.id.id];
		if (offset == 0) {
			[[unlikely]];
			return;
		}
		assert(denseDataEntityIds.size() > offset);
		if (denseDataEntityIds[offset-1] != handle.id) {
			[[unlikely]];
			return;
		}
		const uint32_t last = denseData.size()-1;
		uint32_t id = handle.id.id;
		if (last != offset) {
			[[likely]];
			denseData[offset-1] = std::move(denseData[last-1]);
			denseDataEntityIds[offset-1] = denseDataEntityIds[last-1];
			id = denseDataEntityIds[offset-1].id;
		}
		entityDataOffsets[id] = offset;
		denseDataEntityIds.resize(denseDataEntityIds.size() - 1);
		denseData.resize(denseData.size() - 1);
	}

	T *Get(PeerHandle handle)
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		if (entityDataOffsets.size() >= handle.id.id) {
			[[unlikely]];
			return nullptr;
		}
		const uint32_t offset = entityDataOffsets[handle.id.id];
		if (offset == 0) {
			[[unlikely]];
			return nullptr;
		}
		assert(denseDataEntityIds.size() > offset);
		if (denseDataEntityIds[offset-1] != handle.id) {
			[[unlikely]];
			return nullptr;
		}
		return &(denseData[offset-1]);
	}

	const T *Get(PeerHandle handle) const
	{
		assert(handle.loop == loop);
		assert(denseData.size() == denseDataEntityIds.size());
		if (entityDataOffsets.size() >= handle.id.id) {
			[[unlikely]];
			return nullptr;
		}
		const uint32_t offset = entityDataOffsets[handle.id.id];
		if (offset == 0) {
			[[unlikely]];
			return nullptr;
		}
		assert(denseDataEntityIds.size() > offset);
		if (denseDataEntityIds[offset-1] != handle.id) {
			[[unlikely]];
			return nullptr;
		}
		return &(denseData[offset]);
	}

	T &operator[](PeerHandle handle)
	{
		return Access(handle);
	}

	uint32_t Size() const
	{
		return denseData.size();
	}

public:
	std::vector<uint32_t> entityDataOffsets;
	std::vector<PeerId> denseDataEntityIds;
	std::vector<T> denseData;
	Loop *loop = nullptr;
};

template<typename T>
struct MapPeerHandle
{
	T &Set(PeerHandle handle, T &&value)
	{
		return maps.Access(handle.loop).Set(handle, std::move(value));
	}

	T &Set(PeerHandle handle, const T &value)
	{
		return maps.Access(handle.loop).Set(handle, value);
	}

	T &Access(PeerHandle handle)
	{
		return maps.Access(handle.loop).Access(handle);
	}

	void Remove(PeerHandle handle)
	{
		MapPeerHandleInLoop<T> *ptr = maps.Get(handle.loop);
		if (ptr == nullptr) {
			[[unlikely]];
			return;
		}
		ptr->Remove(handle);
	}

	T *Get(PeerHandle handle)
	{
		MapPeerHandleInLoop<T> *ptr = maps.Get(handle.loop);
		if (ptr == nullptr) {
			[[unlikely]];
			return;
		}
		ptr->Get(handle);
	}

	const T *Get(PeerHandle handle) const
	{
		MapPeerHandleInLoop<T> *ptr = maps.Get(handle.loop);
		if (ptr == nullptr) {
			[[unlikely]];
			return;
		}
		ptr->Get(handle);
	}

	T &operator[](PeerHandle handle)
	{
		return Access(handle);
	}

	uint32_t Size() const
	{
		uint32_t size = 0;
		for (MapPeerHandleInLoop<T> &m : maps) {
			size += maps.Size();
		}
		return size;
	}

public:
	MapLoop<MapPeerHandleInLoop<T>> maps;
};

}

#endif
