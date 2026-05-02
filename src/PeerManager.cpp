// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cassert>

#include "../include/icon7/Peer.hpp"
#include "../include/icon7/Host.hpp"
#include "../include/icon7/Loop.hpp"

#include "../include/icon7/PeerManager.hpp"

namespace icon7
{
PeerManager::PeerManager(Loop *const loop) : loop(loop) {}
PeerManager::~PeerManager() { Clear(); }

void PeerManager::Clear()
{
	std::lock_guard lock(mutex);
	for (auto &p : peers) {
		p = nullptr;
	}
	peers.clear();
	versions.clear();
	freeIds.clear();
}

PeerHandle PeerManager::Alloc(std::shared_ptr<PeerData> peerData)
{
	assert(peerData);
	assert(!peerData->peerHandle);
	std::lock_guard lock(mutex);
	PeerHandle handle;
	handle.loop = loop;
	if (freeIds.size() > 0) {
		handle.id = freeIds.back();
		freeIds.pop_back();
		handle.version = versions[handle.id];
		assert(peers[handle.id] == nullptr);
		peers[handle.id] = peerData;
	} else {
		handle.id = peers.size();
		peers.push_back(peerData);
		handle.version = 1;
		versions.push_back(1);
		perHostOffset.push_back(-1);
	}
	peerData->peerHandle = handle;
	peerData->sharedPeer->peerHandle = handle;
	assert(handle.version != 0);
	return handle;
}

[[nodiscard]] std::shared_ptr<PeerData> PeerManager::Release(PeerHandle handle)
{
	if (ExistsLocal(handle) == false) {
		return nullptr;
	}
	std::lock_guard lock(mutex);
	freeIds.push_back(handle.id);
	std::shared_ptr<PeerData> const data = peers[handle.id];
	peers[handle.id] = nullptr;
	versions[handle.id]++;
	if (versions[handle.id] == 0) {
		versions[handle.id]++;
	}
	peersToFlush.RemovePeerToFlush(handle.id);
	return data;
}

PeerHandle PeerManager::GetCurrentVersionLocal(PeerHandle handle)
{
	assert(peers.size() == versions.size());
	assert(handle.loop == loop);
	if (handle.id >= versions.size()) {
		return {};
	}
	if (peers[handle.id] == nullptr) {
		return {};
	}
	return {.id = handle.id, .version = versions[handle.id], .loop = loop};
}
bool PeerManager::ExistsLocal(PeerHandle handle)
{
	assert(peers.size() == versions.size());
	assert(handle.loop == loop);
	if (handle.id >= versions.size()) {
		return false;
	}
	if (peers[handle.id] == nullptr) {
		return false;
	}
	return handle.version == versions[handle.id];
}

Host *PeerManager::GetLocalHost(PeerHandle handle)
{
	if (PeerData *data = GetLocalPeerData(handle)) {
		return data->host;
	}
	return nullptr;
}
PeerData *PeerManager::GetLocalPeerData(PeerHandle handle)
{
	if (ExistsLocal(handle)) {
		return peers[handle.id].get();
	}
	return nullptr;
}
PeerData *PeerManager::GetLocalPeerDataValid(uint32_t peerId)
{
	assert(peerId < peers.size());
	return peers[peerId].get();
}

PeerHandle PeerManager::GetCurrentVersionShared(PeerHandle handle)
{
	std::shared_lock lock(mutex);
	return GetCurrentVersionLocal(handle);
}
bool PeerManager::ExistsShared(PeerHandle handle)
{
	std::shared_lock lock(mutex);
	return ExistsLocal(handle);
}

std::shared_ptr<Peer> PeerManager::GetSharedPeer(PeerHandle handle)
{
	std::shared_lock lock(mutex);
	PeerData *data = GetLocalPeerData(handle);
	if (data) {
		return data->sharedPeer;
	}
	return nullptr;
}
std::shared_ptr<Host> PeerManager::GetSharedHost(PeerHandle handle)
{
	std::shared_lock lock(mutex);
	if (Host *host = GetLocalHost(handle)) {
		return host->shared_from_this();
	}
	return nullptr;
}

PeerReferences::PeerReferences(Host *const host, Loop *loop)
	: peerManager(&(loop->peerManager)), host(host)
{
}
PeerReferences::~PeerReferences() {}

PeerHandle PeerReferences::Alloc(std::shared_ptr<PeerData> peerData)
{
	PeerHandle handle = peerManager->Alloc(peerData);
	peerManager->perHostOffset[handle.id] = peers.size();
	peers.push_back(peerData);
	return handle;
}
[[nodiscard]] std::shared_ptr<PeerData> PeerReferences::Release(PeerHandle handle)
{
	std::shared_ptr<PeerData> data = peerManager->Release(handle);
	if (data == nullptr) {
		return nullptr;
	}
	assert(data->host == host);
	const int32_t off = peerManager->perHostOffset[handle.id];
	peerManager->perHostOffset[handle.id] = -1;
	if (off + 1 != peers.size()) {
		std::swap(peers[off], peers.back());
		std::shared_ptr<PeerData> last = peers.back();
		peerManager->perHostOffset[peers[off]->peerHandle.id] = off;
		peers.back() = nullptr;
	}
	peers.resize(peers.size() - 1);
	return data;
}
} // namespace icon7
