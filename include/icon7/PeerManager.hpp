// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_PEER_MANAGER_HPP
#define ICON7_PEER_MANAGER_HPP

#include <vector>
#include <shared_mutex>

#include "PeersToFlush.hpp"
#include "Forward.hpp"

namespace icon7
{
class PeerManager
{
public: // can be used only from within networking loop's thread
	PeerManager(Loop *const loop);
	~PeerManager();

	void Clear();

	PeerHandle Alloc(std::shared_ptr<PeerData> peerData);
	[[nodiscard]] std::shared_ptr<PeerData> Release(PeerHandle handle);

	PeerHandle GetCurrentVersionLocal(PeerHandle handle);
	bool ExistsLocal(PeerHandle handle);

	Host *GetLocalHost(PeerHandle handle);
	PeerData *GetLocalPeerData(PeerHandle handle);
	PeerData *GetLocalPeerDataValid(uint32_t peerId);

public: // can be used from any thread
	PeerHandle GetCurrentVersionShared(PeerHandle handle);
	bool ExistsShared(PeerHandle handle1);

	std::shared_ptr<Peer> GetSharedPeer(PeerHandle handle);
	std::shared_ptr<Host> GetSharedHost(PeerHandle handle);

protected:
	friend class PeerReferences;
	friend class Loop;
	friend class Host;
	std::vector<int32_t> perHostOffset;
	PeersToFlush peersToFlush;

private:
	std::vector<std::shared_ptr<PeerData>> peers;
	std::vector<uint32_t> versions;
	std::vector<uint32_t> freeIds;
	Loop *const loop;

	uint8_t _padding1[128];
	std::shared_mutex mutex;
	uint8_t _padding2[128];
};

class PeerReferences
{
public:
	PeerReferences(Host *const host, Loop *loop);
	~PeerReferences();

	PeerHandle Alloc(std::shared_ptr<PeerData> peerData);
	[[nodiscard]] std::shared_ptr<PeerData> Release(PeerHandle handle);

public:
	PeerManager *const peerManager;

protected:
	friend class Host;
	Host *const host;
	std::vector<std::shared_ptr<PeerData>> peers;
};
}

#endif
