/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
 *
 *  ICon7 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ICON7_PEER_HPP
#define ICON7_PEER_HPP

#include <atomic>
#include <memory>
#include <vector>

#include "../../concurrentqueue/concurrentqueue.h"

#include "ConcurrentQueueTraits.hpp"
#include "Command.hpp"
#include "FrameDecoder.hpp"
#include "SendFrameStruct.hpp"
#include "ByteBuffer.hpp"

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

protected:
	Peer(Host *host);

	inline const static uint32_t BIT_READY = 1;
	inline const static uint32_t BIT_DISCONNECTING = 2;
	inline const static uint32_t BIT_CLOSED = 4;
	inline const static uint32_t BIT_ERROR_CONNECT = 8;

protected:
	moodycamel::ConcurrentQueue<ByteBufferStorageHeader *,
								ConcurrentQueueDefaultTraits>
		queue;
	moodycamel::ConsumerToken consumerToken;
	std::atomic<uint32_t> sendingQueueSize;
	std::atomic<uint32_t> peerFlags;

	void (*onDisconnect)(Peer *);

	std::vector<SendFrameStruct> localQueue;

	FrameDecoder frameDecoder;
	uint32_t localQueueOffset;
	const static inline uint32_t MAX_LOCAL_QUEUE_SIZE = 128;
};
} // namespace icon7

#endif
