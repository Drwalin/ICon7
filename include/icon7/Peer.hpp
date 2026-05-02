// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_PEER_HPP
#define ICON7_PEER_HPP

#include <atomic>
#include <memory>
#include <vector>

#include "Stats.hpp"
#include "FrameDecoder.hpp"
#include "Forward.hpp"
#include "OnReturnCallback.hpp"
#include "SendFrameStruct.hpp"
#include "FlatHashMap.hpp"

namespace icon7
{
class Host;
class Peer;
class PeerData;

class RPCEnvironment;
namespace commands
{
namespace internal
{
class CommandCallSend;
}
} // namespace commands

enum PeerStatusBits : uint32_t {
	BIT_READY = 1,
	BIT_DISCONNECTING = 2,
	BIT_CLOSED = 4,
	BIT_ERROR_CONNECT = 8,
};

class Peer : public std::enable_shared_from_this<Peer>
{
public:
	~Peer() = default;

	Peer(Peer &&) = delete;
	Peer(Peer &) = delete;
	Peer(const Peer &) = delete;
	Peer &operator=(Peer &&) = delete;
	Peer &operator=(Peer &) = delete;
	Peer &operator=(const Peer &) = delete;

	static void Send(PeerHandle peer, ByteBufferReadable &frame);
	static void Send(PeerHandle peer, ByteBufferReadable &&frame);
	void Send(ByteBufferReadable &frame);
	void Send(ByteBufferReadable &&frame);
	void Disconnect();
	static void Disconnect(PeerHandle peer);

	inline bool IsReadyToUse() const { return peerFlags & BIT_READY; }
	inline bool IsDisconnecting() const
	{
		return peerFlags & BIT_DISCONNECTING;
	}
	inline bool IsClosed() const { return peerFlags & BIT_CLOSED; }
	inline bool HadConnectError() const
	{
		return peerFlags & BIT_ERROR_CONNECT;
	}

	void SetOnDisconnect(void (*callback)(PeerHandle));

public:
	uint64_t userData;
	void *userPointer;

public:
	Host *const host;
	Loop *const loop;
	bool isClient;
	PeerHandle peerHandle;

	inline uint32_t GetPeerStateFlags() const { return peerFlags.load(); }

protected:
	void _InternalErrorSendOnDisconnecting();

protected:
	Peer(Host *host);

	inline const static uint32_t BIT_READY = 1;
	inline const static uint32_t BIT_DISCONNECTING = 2;
	inline const static uint32_t BIT_CLOSED = 4;
	inline const static uint32_t BIT_ERROR_CONNECT = 8;

	friend class PeerData;
	friend class Host;
	friend class uS::tcp::Peer;
protected:
	std::atomic<uint32_t> peerFlags;

public:
	icon7::PeerStats stats;
};

// Thead unsafe structure, only for internal in-loop operations
class PeerData : public std::enable_shared_from_this<PeerData>
{
public:
	virtual ~PeerData();

	PeerData(PeerData &&) = delete;
	PeerData(PeerData &) = delete;
	PeerData(const PeerData &) = delete;
	PeerData &operator=(PeerData &&) = delete;
	PeerData &operator=(PeerData &) = delete;
	PeerData &operator=(const PeerData &) = delete;

	virtual void Send(ByteBufferReadable &frame);
	virtual void Send(ByteBufferReadable &&frame);

	inline bool IsReadyToUse() const { return sharedPeer->peerFlags & BIT_READY; }
	inline bool IsDisconnecting() const
	{
		return sharedPeer->peerFlags & BIT_DISCONNECTING;
	}
	inline bool IsClosed() const { return sharedPeer->peerFlags & BIT_CLOSED; }
	inline bool HadConnectError() const
	{
		return sharedPeer->peerFlags & BIT_ERROR_CONNECT;
	}

	void SetOnDisconnect(void (*callback)(PeerHandle)) { onDisconnect = callback; }

public: // thread unsafe, safe only in hosts loop thread
	void _InternalOnData(uint8_t *data, uint32_t length);
	void _InternalOnDisconnect();
	bool _InternalOnWritable();
	void _InternalOnTimeout();
	void _InternalOnLongTimeout();

	virtual bool _InternalHasQueuedSends() const;
	virtual bool _InternalHasBufferedSends() const = 0;

	// returns false for cork/error
	virtual bool _InternalFlushBufferedSends(bool hasMore) = 0;

public:
	std::shared_ptr<Peer> sharedPeer;
	PeerHandle peerHandle;
	Host *host;
	Loop *loop;

public:
	void SetReadyToUse();

	inline uint32_t GetPeerStateFlags() const { return sharedPeer->peerFlags; }

protected:
	// returns false for cork/error
	virtual bool _InternalFlushQueuedSends(int maxPacketsToSend);
	void DequeueToLocalQueue();
	/*
	 * return true if successfully whole dataFrame has been sent.
	 * false otherwise.
	 */
	virtual bool _InternalSend(SendFrameStruct &dataFrame, bool hasMore) = 0;
	virtual void _InternalDisconnect() = 0;

	virtual void _InternalClearInternalDataOnClose();

	static void _Internal_static_OnPacket(ByteBufferReadable &buffer,
										  uint32_t headerSize, void *peer);
	void _InternalOnPacket(ByteBufferReadable &buffer, uint32_t headerSize);
	void _InternalOnPacketWithControllSequence(ByteBufferReadable &buffer,
											   uint32_t headerSize);
	virtual void
	_InternalOnPacketWithControllSequenceBackend(ByteBufferReadable &buffer,
												 uint32_t headerSize);

	uint32_t _InternalGetNextValidReturnCallbackId();

	void CheckForTimeoutFunctionCalls(time::Point now);

	friend class Host;
	friend class commands::internal::ExecuteDisconnect;
	friend class RPCEnvironment;
	friend class commands::internal::CommandCallSend;
	friend class uS::tcp::Peer;

protected:
	PeerData(Host *host);

protected:
	void (*onDisconnect)(PeerHandle);
	uint32_t returnIdGen = 0;

	std::vector<SendFrameStruct> globalQueue;
	std::vector<SendFrameStruct> localQueue;

	FrameDecoder frameDecoder;
	uint32_t localQueueOffset;
	const static inline uint32_t MAX_LOCAL_QUEUE_SIZE = 128;

	FlatHashMap<uint32_t, OnReturnCallback, Hash<uint32_t>> returningCallbacks;
};
} // namespace icon7

#endif
