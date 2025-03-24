// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../concurrentqueue/concurrentqueue.h"

#include "../include/icon7/ConcurrentQueueTraits.hpp"
#include "../include/icon7/Host.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/SendFrameStruct.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Flags.hpp"

#include "../include/icon7/Peer.hpp"

namespace icon7
{

Peer::Peer(Host *host)
	: host(host),
	  queue(new moodycamel::ConcurrentQueue<ByteBufferStorageHeader *,
											ConcurrentQueueDefaultTraits>()),
	  consumerToken(new moodycamel::ConsumerToken(*queue))
{
	userData = 0;
	userPointer = nullptr;
	onDisconnect = nullptr;
	peerFlags = 0;
	sendingQueueSize = 0;
	localQueue.reserve(192);
	localQueueOffset = 0;
}

Peer::~Peer()
{
	for (int i = localQueueOffset; i < localQueue.size(); ++i) {
		if (localQueue[localQueueOffset].data.storage) {
			localQueue[localQueueOffset].data.storage->unref();
			localQueue[localQueueOffset].data.storage = nullptr;
		}
	}
	localQueueOffset = 0;
	ByteBufferStorageHeader *ar[MAX_LOCAL_QUEUE_SIZE];
	for (int i = 0; i < 16; ++i) {
		while (true) {
			size_t dequeued = queue->try_dequeue_bulk(*consumerToken, ar,
													  MAX_LOCAL_QUEUE_SIZE);
			if (dequeued == 0) {
				break;
			}
			for (int i = 0; i < dequeued; ++i) {
				ar[i]->unref();
			}
		}
	}

	delete queue;
	queue = nullptr;
	delete consumerToken;
	consumerToken = nullptr;
}

void Peer::Send(ByteBuffer &frame)
{
	if (IsDisconnecting()) {
		LOG_WARN("TRY SEND IN DISCONNECTING PEER, dropping packet");
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	frame.storage->ref();
	queue->enqueue(frame.storage);
	// host->InsertPeerToFlush(this);
}

void Peer::SendLocalThread(ByteBuffer &frame)
{
	if (IsDisconnecting()) {
		LOG_WARN("TRY SEND IN DISCONNECTING PEER, dropping packet");
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	localQueue.push_back(frame);
	// host->_InternalInsertPeerToFlush(this);
}

void Peer::Send(ByteBuffer &&frame)
{
	if (IsDisconnecting()) {
		LOG_WARN("TRY SEND IN DISCONNECTING PEER, dropping packet");
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	queue->enqueue(frame.storage);
	frame.storage = nullptr;
	// host->InsertPeerToFlush(this);
}

void Peer::SendLocalThread(ByteBuffer &&frame)
{
	if (IsDisconnecting()) {
		LOG_WARN("TRY SEND IN DISCONNECTING PEER, dropping packet");
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	localQueue.emplace_back(std::move(frame));
	// host->_InternalInsertPeerToFlush(this);
}

void Peer::Disconnect()
{
	peerFlags |= BIT_DISCONNECTING;
	auto com = CommandHandle<commands::internal::ExecuteDisconnect>::Create();
	com->peer = shared_from_this();
	host->EnqueueCommand(std::move(com));
}

void Peer::_InternalOnData(uint8_t *data, uint32_t length)
{
	frameDecoder.PushData(data, length, _Internal_static_OnPacket, this);
}

void Peer::_Internal_static_OnPacket(ByteBuffer &buffer, uint32_t headerSize,
									 void *peer)
{
	((Peer *)peer)->_InternalOnPacket(buffer, headerSize);
}

void Peer::_InternalOnPacket(ByteBuffer &buffer, uint32_t headerSize)
{
	if (buffer.size() == headerSize) {
		LOG_ERROR("Protocol doesn't allow for 0 sized packets.");
		return;
	}
	if ((FramingProtocol::GetPacketFlags(buffer.data(), 0) &
		 FLAGS_PROTOCOL_CONTROLL_SEQUENCE) ==
		FLAGS_PROTOCOL_CONTROLL_SEQUENCE) {
		_InternalOnPacketWithControllSequence(buffer, headerSize);
	} else {
		host->GetRpcEnvironment()->OnReceive(this, buffer, headerSize,
											 FLAG_RELIABLE);
	}
}

void Peer::_InternalOnPacketWithControllSequence(ByteBuffer &buffer,
												 uint32_t headerSize)
{
	uint8_t vectorCall = buffer.data()[headerSize];
	if (vectorCall <= 0x7F) {
		// TODO: decode here future controll sequences
		uint32_t vectorCall = buffer.data()[headerSize];
		LOG_WARN("Received packet with undefined controll sequence: 0x%X",
				 vectorCall);
	} else {
		_InternalOnPacketWithControllSequenceBackend(buffer, headerSize);
	}
}

void Peer::_InternalOnPacketWithControllSequenceBackend(ByteBuffer &buffer,
														uint32_t headerSize)
{
	uint32_t vectorCall = buffer.data()[headerSize];
	LOG_WARN("Unhandled packet with controll sequence by backend. Vector call "
			 "value: 0x%X",
			 vectorCall);
}

bool Peer::_InternalOnWritable()
{
	if (!IsReadyToUse() || IsDisconnecting() || IsClosed()) {
		return false;
	}
	bool ret = true;
	if (_InternalHasQueuedSends()) {
		ret = _InternalFlushQueuedSends();
	}
	if (_InternalHasBufferedSends()) {
		ret &= _InternalFlushBufferedSends(false);
	}
	return ret;
}

void Peer::_InternalOnDisconnect()
{
	if (onDisconnect) {
		onDisconnect(this);
	} else if (host->onDisconnect) {
		host->onDisconnect(this);
	}
}

void Peer::_InternalOnTimeout() { _InternalDisconnect(); }

void Peer::_InternalOnLongTimeout() { _InternalDisconnect(); }

bool Peer::_InternalHasQueuedSends() const { return sendingQueueSize != 0; }

void Peer::SetReadyToUse() { peerFlags |= BIT_READY; }

void Peer::DequeueToLocalQueue()
{
	if (localQueue.size() != localQueueOffset) {
		return;
	}

	ByteBufferStorageHeader *ar[MAX_LOCAL_QUEUE_SIZE];
	uint32_t dequeued =
		queue->try_dequeue_bulk(*consumerToken, ar, MAX_LOCAL_QUEUE_SIZE);
	localQueue.resize(dequeued);
	ByteBuffer buffer;
	for (int i = 0; i < dequeued; ++i) {
		buffer.storage = ar[i];
		localQueue[i] = std::move(buffer);
	}
	localQueueOffset = 0;
}

bool Peer::_InternalFlushQueuedSends()
{
	bool ret = true;
	const uint32_t queuedFrames = sendingQueueSize.load();
	uint32_t sentFrames = 0;
	for (uint32_t i = 0; i < 300; ++i) {
		if (localQueue.size() == localQueueOffset) {
			DequeueToLocalQueue();
		}
		if (localQueue.size() == localQueueOffset) {
			ret = true;
			break;
		}
		const bool hasAnyMore = queuedFrames - sentFrames > 1;
		if (_InternalSend(localQueue[localQueueOffset], hasAnyMore)) {
			sentFrames++;
			localQueue[localQueueOffset].data.storage->unref();
			localQueue[localQueueOffset].data.storage = nullptr;
			localQueueOffset++;
		} else {
			ret = false;
			break;
		}
	}
	sendingQueueSize -= sentFrames;
	return ret;
}

void Peer::_InternalClearInternalDataOnClose() { peerFlags |= BIT_CLOSED; }

} // namespace icon7
