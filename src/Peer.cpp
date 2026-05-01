// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Host.hpp"
#include "../include/icon7/Loop.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/Time.hpp"

#include "../include/icon7/Peer.hpp"

namespace icon7
{
Peer::Peer(Host *host) : host(host), loop(host->loop.get())
{
	userData = 0;
	userPointer = nullptr;
	peerFlags = 0;
}

PeerData::PeerData(Host *host) : host(host), loop(host->loop.get())
{
	struct PeerPublic : public Peer {
		PeerPublic(Host *host) : Peer(host) {}
		virtual ~PeerPublic() {}
	};
	sharedPeer = std::make_shared<PeerPublic>(host);
// 	peerHandle = {shared_from_this()};
// 	sharedPeer->peerHandle = peerHandle;
	onDisconnect = nullptr;
	localQueue.reserve(32);
	globalQueue.reserve(32);
	localQueueOffset = 0;
}

PeerData::~PeerData()
{
	for (int i = localQueueOffset; i < localQueue.size(); ++i) {
		if (localQueue[i].data.storage) {
			localQueue[i].data.storage->unref();
			localQueue[i].data.storage = nullptr;
		}
	}
	for (int i = 0; i < globalQueue.size(); ++i) {
		if (globalQueue[i].data.storage) {
			globalQueue[i].data.storage->unref();
			globalQueue[i].data.storage = nullptr;
		}
	}
}

void Peer::Send(ByteBufferReadable &frame)
{
	assert(frame.data());
	ByteBufferReadable frameLocal = frame;
	Send(std::move(frameLocal));
}

void Peer::Send(ByteBufferReadable &&frame)
{
	assert(frame.data());
	if (IsDisconnecting()) {
		_InternalErrorSendOnDisconnecting();
		return;
	}
	auto com =
		CommandHandle<commands::internal::ExecutePeerSendFrame>::Create(peerHandle);
	com->frame = std::move(frame);
	host->EnqueueCommand(std::move(com));
	assert(frame.storage == nullptr);
}

void Peer::Send(PeerHandle peer, ByteBufferReadable &frame)
{
	assert(frame.data());
	ByteBufferReadable frameLocal = frame;
	Send(peer, std::move(frameLocal));
}

void Peer::Send(PeerHandle peer, ByteBufferReadable &&frame)
{
	assert(frame.data());
	auto com =
		CommandHandle<commands::internal::ExecutePeerSendFrame>::Create(peer);
	com->frame = std::move(frame);
	peer.GetLoop()->EnqueueCommand(std::move(com));
	assert(frame.storage == nullptr);
}

void PeerData::Send(ByteBufferReadable &frame)
{
	assert(frame.data());
	ByteBufferReadable frameLocal = frame;
	Send(std::move(frameLocal));
}

void PeerData::Send(ByteBufferReadable &&frame)
{
	assert(frame.data());
	FramingProtocol::PrintDetailsAboutFrame(frame);
	if (IsDisconnecting()) {
		sharedPeer->_InternalErrorSendOnDisconnecting();
		return;
	}
	if (_InternalHasQueuedSends() == false &&
		_InternalHasBufferedSends() == false) {
		host->_InternalInsertPeerToFlush(peerHandle);
	}
	globalQueue.emplace_back(std::move(frame));
	assert(frame.storage == nullptr);
}

void Peer::_InternalErrorSendOnDisconnecting()
{
	stats.errorsCount += 1;
	host->stats.errorsCount += 1;
	host->loop->stats.errorsCount += 1;
	static time::Point last = time::GetTemporaryTimestamp();
	if (last + time::seconds(1) < time::GetTemporaryTimestamp() ||
		last - time::seconds(10) > time::GetTemporaryTimestamp()) {
		last = time::GetTemporaryTimestamp();
		LOG_WARN("TRY SEND IN DISCONNECTING PEER, dropping packet");
	}
}

void Peer::Disconnect()
{
	peerFlags |= BIT_DISCONNECTING;
	Disconnect(peerHandle);
}

void Peer::Disconnect(PeerHandle peer)
{
	auto com = CommandHandle<commands::internal::ExecuteDisconnect>::Create(peer);
	peer.GetLoop()->EnqueueCommand(std::move(com));
}

void PeerData::_InternalOnData(uint8_t *data, uint32_t length)
{
	sharedPeer->stats.bytesReceived += length;
	host->stats.bytesReceived += length;
	host->loop->stats.bytesReceived += length;

	sharedPeer->stats.packetsReceived += 1;
	host->stats.packetsReceived += 1;
	host->loop->stats.packetsReceived += 1;

	frameDecoder.PushData(data, length, _Internal_static_OnPacket, this);
}

void PeerData::_Internal_static_OnPacket(ByteBufferReadable &buffer, uint32_t headerSize,
									 void *peer)
{
	((PeerData *)peer)->_InternalOnPacket(buffer, headerSize);
}

void PeerData::_InternalOnPacket(ByteBufferReadable &buffer, uint32_t headerSize)
{
	sharedPeer->stats.framesReceived += 1;
	host->stats.framesReceived += 1;
	host->loop->stats.framesReceived += 1;

	if (buffer.size() == headerSize) {
		sharedPeer->stats.errorsCount += 1;
		host->stats.errorsCount += 1;
		host->loop->stats.errorsCount += 1;
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

void PeerData::_InternalOnPacketWithControllSequence(ByteBufferReadable &buffer,
												 uint32_t headerSize)
{
	uint8_t vectorCall = buffer.data()[headerSize];
	if (vectorCall <= 0x7F) {
		// TODO: decode here future controll sequences
		uint32_t vectorCall = buffer.data()[headerSize];
		sharedPeer->stats.errorsCount += 1;
		host->stats.errorsCount += 1;
		host->loop->stats.errorsCount += 1;
		LOG_WARN("Received packet with undefined controll sequence: 0x%X",
				 vectorCall);
	} else {
		_InternalOnPacketWithControllSequenceBackend(buffer, headerSize);
	}
}

void PeerData::_InternalOnPacketWithControllSequenceBackend(ByteBufferReadable &buffer,
														uint32_t headerSize)
{
	uint32_t vectorCall = buffer.data()[headerSize];
	sharedPeer->stats.errorsCount += 1;
	host->stats.errorsCount += 1;
	host->loop->stats.errorsCount += 1;
	LOG_WARN("Unhandled packet with controll sequence by backend. Vector call "
			 "value: 0x%X",
			 vectorCall);
}

bool PeerData::_InternalOnWritable()
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

void PeerData::_InternalOnDisconnect()
{
	if (onDisconnect) {
		onDisconnect(peerHandle);
	} else if (host->onDisconnect) {
		host->onDisconnect(peerHandle);
	}
}

void PeerData::_InternalOnTimeout() { _InternalDisconnect(); }

void PeerData::_InternalOnLongTimeout() { _InternalDisconnect(); }

bool PeerData::_InternalHasQueuedSends() const
{
	return localQueueOffset != localQueue.size() || globalQueue.size() != 0;
}

void PeerData::SetReadyToUse() { sharedPeer->peerFlags |= BIT_READY; }

void PeerData::DequeueToLocalQueue()
{
	if (localQueue.size() != localQueueOffset) {
		return;
	}
	if (localQueueOffset != 0) {
		localQueueOffset = 0;
		localQueue.clear();
	}
	if (globalQueue.empty()) {
		return;
	}
	std::swap(localQueue, globalQueue);
}

bool PeerData::_InternalFlushQueuedSends()
{
	bool ret = true;
	const uint32_t queuedFrames =
		globalQueue.size() + localQueue.size() - localQueueOffset;
	uint32_t sentFrames = 0;
	uint32_t i = 0;
	for (int I = 0; I < 2; ++I) {
		DequeueToLocalQueue();
		for (; i < 300; ++i) {
			if (localQueue.size() == localQueueOffset) {
				ret = true;
				break;
			}
			const bool hasAnyMore = (queuedFrames - sentFrames) > 1;
			if (_InternalSend(localQueue[localQueueOffset], hasAnyMore)) {
				sentFrames++;
				localQueue[localQueueOffset].data.storage->unref();
				localQueue[localQueueOffset].data.storage = nullptr;
				localQueueOffset++;
			} else {
				ret = false;
				I = 3;
				break;
			}
		}
	}

	sharedPeer->stats.framesSent += sentFrames;
	host->stats.framesSent += sentFrames;
	host->loop->stats.framesSent += sentFrames;

	return ret;
}

uint32_t PeerData::_InternalGetNextValidReturnCallbackId()
{
	uint32_t id;
	do {
		id = ++returnIdGen;
	} while (returningCallbacks.Has(id) || id == 0);
	return id;
}

void PeerData::_InternalClearInternalDataOnClose() {
	sharedPeer->peerFlags |= BIT_CLOSED;
	sharedPeer = {};
	peerHandle = {};
}

void PeerData::CheckForTimeoutFunctionCalls(time::Point now)
{
	auto *it = returningCallbacks.GetRandom();
	if (it == nullptr) {
		return;
	}
	OnReturnCallback &cb = it->v;

	if (cb.IsExpired(now)) {
		cb.ExecuteTimeout();
		returningCallbacks.Remove(it->key);
	}
}
} // namespace icon7
