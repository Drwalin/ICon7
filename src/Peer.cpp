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

#include "../include/icon7/Host.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Flags.hpp"

#include "../include/icon7/Peer.hpp"

namespace icon7
{

Peer::Peer(Host *host) : host(host), queue(), consumerToken(queue)
{
	userData = 0;
	userPointer = nullptr;
	onDisconnect = nullptr;
	peerFlags = 0;
	sendingQueueSize = 0;
	localQueueSize = 0;
	localQueueOffset = 0;
	localQueue = new SendFrameStruct[MAX_LOCAL_QUEUE_SIZE];
}

Peer::~Peer()
{
	for (int i = 0; i < MAX_LOCAL_QUEUE_SIZE; ++i) {
		localQueue[i].~SendFrameStruct();
	}
	localQueueOffset = 0;
	localQueueSize = 0;
	ByteBufferStorageHeader *ar[MAX_LOCAL_QUEUE_SIZE];
	for (int i = 0; i < 16; ++i) {
		while (true) {
			size_t dequeued =
				queue.try_dequeue_bulk(consumerToken, ar, MAX_LOCAL_QUEUE_SIZE);
			if (dequeued == 0) {
				break;
			}
			for (int i = 0; i < dequeued; ++i) {
				ar[i]->unref();
			}
		}
	}
	delete[] localQueue;
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
	queue.enqueue(frame.storage);
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
	frame.storage->ref();
	// replace with push to local queue
	queue.enqueue(frame.storage);
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

void Peer::_InternalOnWritable()
{
	if (IsClosed()) {
		return;
	}
	_InternalFlushQueuedSends();
}

void Peer::_InternalOnDisconnect()
{
	if (onDisconnect) {
		onDisconnect(this);
	} else if (host->onDisconnect) {
		host->onDisconnect(this);
	}
}

void Peer::_InternalOnTimeout() {}

void Peer::_InternalOnLongTimeout() { _InternalDisconnect(); }

bool Peer::_InternalHasQueuedSends() const { return sendingQueueSize != 0; }

void Peer::SetReadyToUse() { peerFlags |= BIT_READY; }

void Peer::DequeueToLocalQueue()
{
	if (localQueueSize != localQueueOffset) {
		return;
	}

	ByteBufferStorageHeader *ar[MAX_LOCAL_QUEUE_SIZE];
	localQueueSize =
		queue.try_dequeue_bulk(consumerToken, ar, MAX_LOCAL_QUEUE_SIZE);
	for (int i = 0; i < localQueueSize; ++i) {
		ByteBuffer buffer;
		buffer.storage = ar[i];
		localQueue[i] = std::move(buffer);
	}
	localQueueOffset = 0;
}

void Peer::_InternalFlushQueuedSends()
{
	for (uint32_t i = 0; i < 16; ++i) {
		DequeueToLocalQueue();
		if (localQueueSize == localQueueOffset) {
			break;
		}

		auto &frame = localQueue[localQueueOffset].data;
		if (frame.size() > 17) {
			LOG_DEBUG("Trying to send invalid frame: "
					  "size %i   cap %i   ref %i   off %i",
					  frame.size(), frame.capacity(),
					  frame.storage->refCounter.load(), frame.storage->offset);
		}
		if (_InternalSend(localQueue[localQueueOffset], sendingQueueSize > 1)) {
			sendingQueueSize--;
			localQueue[localQueueOffset].data.storage->unref();
			localQueue[localQueueOffset].data.storage = nullptr;
			localQueueOffset++;
		} else {
			break;
		}
	}
}

void Peer::_InternalClearInternalDataOnClose()
{
	this->peerFlags |= BIT_CLOSED;
}

} // namespace icon7
