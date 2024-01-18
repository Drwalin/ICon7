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

#define ICON7_INCLUDE_CONCURRENT_QUEUE_CPP

#include "../include/icon7/Host.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/CommandExecutionQueue.hpp"
#include "../include/icon7/FramingProtocol.hpp"

#include "../include/icon7/Peer.hpp"

namespace icon7
{

Peer::Peer(Host *host) : host(host)
{
	userData = 0;
	userPointer = nullptr;
	onDisconnect = nullptr;
	peerFlags = 0;
	sendQueue = new QueueType();
	sendingQueueSize = 0;
}

Peer::~Peer()
{
	delete sendQueue;
	sendQueue = nullptr;
}

void Peer::Send(std::vector<uint8_t> &&dataWithoutHeader, Flags flags)
{
	if (IsDisconnecting()) {
		// TODO: inform about dropping packets
		return;
	}
	sendingQueueSize++;
	sendQueue->enqueue(SendFrameStruct(std::move(dataWithoutHeader), flags));
	host->InsertPeerToFlush(this);
}

void Peer::Disconnect()
{
	peerFlags |= BIT_DISCONNECTING;
	Command command{commands::ExecuteDisconnect{}};
	commands::ExecuteDisconnect &com =
		std::get<commands::ExecuteDisconnect>(command.cmd);
	com.peer = shared_from_this();
	host->EnqueueCommand(std::move(command));
}

void Peer::_InternalOnData(uint8_t *data, uint32_t length)
{
	frameDecoder.PushData(data, length, _Internal_static_OnPacket, this);
}

void Peer::_Internal_static_OnPacket(std::vector<uint8_t> &buffer,
									 uint32_t headerSize, void *peer)
{
	((Peer *)peer)->_InternalOnPacket(buffer, headerSize);
}

void Peer::_InternalOnPacket(std::vector<uint8_t> &buffer, uint32_t headerSize)
{
	host->GetRpcEnvironment()->OnReceive(this, buffer, headerSize,
										 FLAG_RELIABLE);
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

void Peer::_InternalClearInternalDataOnClose() {}
} // namespace icon7
