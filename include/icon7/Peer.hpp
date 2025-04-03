// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
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

namespace icon7
{
class Host;

class Peer : public std::enable_shared_from_this<Peer>
{
public:
	virtual ~Peer();

	Peer(Peer &&) = delete;
	Peer(Peer &) = delete;
	Peer(const Peer &) = delete;
	Peer &operator=(Peer &&) = delete;
	Peer &operator=(Peer &) = delete;
	Peer &operator=(const Peer &) = delete;

	virtual void Send(ByteBuffer &frame);
	virtual void SendLocalThread(ByteBuffer &frame);
	virtual void Send(ByteBuffer &&frame);
	virtual void SendLocalThread(ByteBuffer &&frame);
	void Disconnect();

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

	void SetOnDisconnect(void (*callback)(Peer *)) { onDisconnect = callback; }

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
	uint64_t userData;
	void *userPointer;

public:
	Host *const host;
	bool isClient;

	void SetReadyToUse();

	inline uint32_t GetPeerStateFlags() const { return peerFlags.load(); }

protected:
	// returns false for cork/error
	virtual bool _InternalFlushQueuedSends();
	void DequeueToLocalQueue();
	/*
	 * return true if successfully whole dataFrame has been sent.
	 * false otherwise.
	 */
	virtual bool _InternalSend(SendFrameStruct &dataFrame, bool hasMore) = 0;
	virtual void _InternalDisconnect() = 0;

	virtual void _InternalClearInternalDataOnClose();

	static void _Internal_static_OnPacket(ByteBuffer &buffer,
										  uint32_t headerSize, void *peer);
	void _InternalOnPacket(ByteBuffer &buffer, uint32_t headerSize);
	void _InternalOnPacketWithControllSequence(ByteBuffer &buffer,
											   uint32_t headerSize);
	virtual void
	_InternalOnPacketWithControllSequenceBackend(ByteBuffer &buffer,
												 uint32_t headerSize);

	friend class Host;
	friend class commands::internal::ExecuteDisconnect;
	friend class RPCEnvironment;

protected:
	Peer(Host *host);

	inline const static uint32_t BIT_READY = 1;
	inline const static uint32_t BIT_DISCONNECTING = 2;
	inline const static uint32_t BIT_CLOSED = 4;
	inline const static uint32_t BIT_ERROR_CONNECT = 8;

protected:
	std::atomic<uint32_t> sendingQueueSize;
	std::atomic<uint32_t> peerFlags;

	alignas(64) void (*onDisconnect)(Peer *);
	uint32_t returnIdGen = 0;

	std::vector<SendFrameStruct> localQueue;

	FrameDecoder frameDecoder;
	uint32_t localQueueOffset;
	const static inline uint32_t MAX_LOCAL_QUEUE_SIZE = 128;

	moodycamel::ConcurrentQueue<ByteBufferStorageHeader *,
								ConcurrentQueueDefaultTraits> *queue;
	moodycamel::ConsumerToken *consumerToken;

public:

	icon7::PeerStats stats;
};
} // namespace icon7

#endif
