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

#include <cstring>

#include <thread>

#include "../concurrentqueue/concurrentqueue.h"

#define ICON7_PEER_CPP_INCLUDE_UNION_CONCURRENT_QUEUE
#include "../include/icon7/Peer.hpp"
#undef ICON7_PEER_CPP_INCLUDE_UNION_CONCURRENT_QUEUE

#include "../include/icon7/Host.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/FramingProtocol.hpp"

namespace icon7
{

Peer::SendFrameStruct::SendFrameStruct(
	std::vector<uint8_t> &&_dataWithoutHeader, Flags flags)
	: dataWithoutHeader(std::move(_dataWithoutHeader))
{
	this->flags = flags;
	bytesSent = 0;
	headerBytesSent = 0;
	headerSize = FramingProtocol::GetHeaderSize(dataWithoutHeader.size());
	*(uint32_t *)header = 0;
	FramingProtocol::WriteHeader(header, headerSize, dataWithoutHeader.size(),
								 flags);
}
Peer::SendFrameStruct::SendFrameStruct() {}

Peer::Peer(Host *host) : host(host)
{
	userData = 0;
	userPointer = nullptr;
	onDisconnect = nullptr;
	readyToUse = false;
	disconnecting = false;
	closed = false;
	sendQueue = new QueueType();
	sendingQueueSize = 0;
	receivingHeaderSize = 0;
	receivingFrameSize = 0;
}

Peer::~Peer()
{
	delete sendQueue;
	sendQueue = nullptr;
}

void Peer::Send(std::vector<uint8_t> &&dataWithoutHeader, Flags flags)
{
	if (disconnecting) {
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	sendQueue->enqueue(SendFrameStruct(std::move(dataWithoutHeader), flags));
	host->InsertPeerToFlush(this);
}

void Peer::Disconnect()
{
	disconnecting = true;
	Command command{commands::ExecuteDisconnect{}};
	commands::ExecuteDisconnect &com =
		std::get<commands::ExecuteDisconnect>(command.cmd);
	com.peer = shared_from_this();
	host->EnqueueCommand(std::move(command));
}

void Peer::_InternalOnData(uint8_t *data, uint32_t length)
{
	while (length) {
		if (receivingHeaderSize == 0) {
			receivingHeaderSize = FramingProtocol::GetPacketHeaderSize(data[0]);
		}
		if (receivingFrameBuffer.size() < receivingHeaderSize) {
			uint32_t bytes = std::min<uint32_t>(
				length, receivingHeaderSize - receivingFrameBuffer.size());
			receivingFrameBuffer.insert(receivingFrameBuffer.end(), data,
										data + bytes);
			length -= bytes;
			data += bytes;
		}
		if (receivingFrameBuffer.size() < receivingHeaderSize) {
			return;
		}
		if (receivingFrameBuffer.size() == receivingHeaderSize) {
			receivingFrameSize =
				receivingHeaderSize +
				FramingProtocol::GetPacketBodySize(receivingFrameBuffer.data(),
												   receivingHeaderSize);
			receivingFrameBuffer.reserve(receivingFrameSize);
		}

		if (receivingFrameBuffer.size() < receivingFrameSize) {
			uint32_t bytes = std::min<uint32_t>(
				length, receivingFrameSize - receivingFrameBuffer.size());
			receivingFrameBuffer.insert(receivingFrameBuffer.end(), data,
										data + bytes);
			length -= bytes;
			data += bytes;
		}

		if (receivingFrameBuffer.size() == receivingFrameSize) {
			host->GetRpcEnvironment()->OnReceive(
				this, receivingFrameBuffer, receivingHeaderSize, FLAG_RELIABLE);
			receivingFrameSize = 0;
			receivingHeaderSize = 0;
			receivingFrameBuffer.clear();
		} else if (receivingFrameBuffer.size() >= receivingFrameSize) {
			DEBUG("Error, Peer::_InternalOnData push to frame more than frame "
				  "size was.");
			throw;
		} else {
			continue;
		}
	}
}

void Peer::_InternalOnWritable()
{
	if (closed) {
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

void Peer::SetReadyToUse() { readyToUse = true; }

void Peer::_InternalFlushQueuedSends()
{
	_InternalPopQueuedSendsFromAsync();
	for (uint32_t i = 0; i < 16 && sendQueueLocal.empty() == false; ++i) {
		if (_InternalSend(sendQueueLocal.front(), sendingQueueSize > 1)) {
			sendQueueLocal.pop();
			sendingQueueSize--;
		} else {
			break;
		}
		if (sendQueueLocal.empty()) {
			_InternalPopQueuedSendsFromAsync();
		}
	}
	_InternalPopQueuedSendsFromAsync();
}

void Peer::_InternalPopQueuedSendsFromAsync()
{
	const uint32_t MAX_DEQUEUE = 16;
	SendFrameStruct frames[MAX_DEQUEUE];
	uint32_t poped = sendQueue->try_dequeue_bulk(frames, MAX_DEQUEUE);
	for (uint32_t i = 0; i < poped; ++i) {
		sendQueueLocal.push(std::move(frames[i]));
	}
}
} // namespace icon7
