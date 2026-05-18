// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_PEER_HANDLE_HPP
#define ICON7_PEER_HANDLE_HPP

#include <cstdint>

#include <memory>

namespace icon7
{
class ByteBufferReadable;
class Loop;
class Host;
class Peer;
class PeerData;
class CommandsBufferHandler;
class MultiBatchWriter;
} // namespace icon7

namespace icon7
{
struct PeerId
{
	uint32_t id = 0;
	uint32_t version = 0;
	inline bool operator==(const PeerId &r) const
	{
		return id == r.id && version == r.version;
	}
	inline bool operator!=(const PeerId &r) const
	{
		return id != r.id || version != r.version;
	}
};

struct PeerHandle {
	PeerId id;
	Loop *loop = nullptr;
	inline bool operator==(const PeerHandle &r) const
	{
		return id == r.id && loop == r.loop;
	}
	inline bool operator!=(const PeerHandle &r) const
	{
		return id != r.id || loop != r.loop;
	}
	inline operator bool() const { return id.version && loop; }

	// Should be invoked very rarely
	PeerData *GetLocalPeerData();
	std::shared_ptr<Peer> GetSharedPeer();
	Loop *GetLoop();
	Host *GetHost();
	
	void Send(ByteBufferReadable &frame);
	void Send(ByteBufferReadable &&frame);
	void Send(CommandsBufferHandler *commandsBuffer, ByteBufferReadable &frame);
	void Send(CommandsBufferHandler *commandsBuffer, ByteBufferReadable &&frame);
	void Send(MultiBatchWriter *batchWriter, ByteBufferReadable &frame);
	void Send(MultiBatchWriter *batchWriter, ByteBufferReadable &&frame);
	void Disconnect();
};
} // namespace icon7

namespace std
{
template<typename T>
struct hash;
}

template<>
struct std::hash<icon7::PeerHandle>
{
    std::size_t operator()(icon7::PeerHandle peer) const noexcept
    {
		uint64_t h = 0xcbf29ce484222325;
		constexpr uint64_t p = 0x100000001b3;
		h = (h ^ peer.id.id) * p;
		h = (h ^ peer.id.version) * p;
		h = (h ^ uintptr_t(peer.loop)) * p;
		return h;
    }
};

#endif
