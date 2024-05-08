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

#include <memory>

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/HostUStcpUdp.hpp"
#include "../include/icon7/Random.hpp"

#include "../include/icon7/PeerUStcpUdp.hpp"

namespace icon7
{
namespace uS
{
namespace tcpudp
{
Peer::Peer(uS::tcpudp::Host *host, us_socket_t *socket)
	: icon7::uS::tcp::Peer(host, socket),
	  maxUnreliablePacketSize(1200 -
							  (SSL ? 16 + 12 : 0)) // TODO: check sizes for
												   // CHACHA20-POLY1305 nonce
												   // and mac 16+12
{
	tmpUdpPacketCollection.reserve(32);
	tmpUdpPacketCollectionSizes.reserve(32);
	Random::Bytes(sendingKey, 32);
	Random::Bytes(receivingKey, 32);
	hasPeerThisAddress = false;
	hasRemoteAddress = false;
	hasSendingIdentity = false;
	receivingNewestPacketId = 0;
	sendingNewestPacketId = 0;
}

Peer::~Peer()
{
	Random::Bytes(sendingKey, 32);
	Random::Bytes(receivingKey, 32);
}

bool Peer::_InternalSend(SendFrameStruct &dataFrame, bool hasMore)
{
	if (dataFrame.flags & FLAG_UNRELIABLE) {
		uint32_t bytes = dataFrame.GetBytes();
		udpSendFrames.insert(
			std::pair<uint32_t, SendFrameStruct>(bytes, std::move(dataFrame)));
		return true;
	} else {
		return icon7::uS::tcp::Peer::_InternalSend(dataFrame, hasMore);
	}
}

bool Peer::_InternalFlushQueuedSends()
{
	LOG_FATAL("Not implemented yet.");
	return false;
	/*
	Host *host = (Host *)(this->host);
	icon7::uS::tcp::Peer::_InternalFlushQueuedSends();

	LOG_DEBUG("%s %s %s",
			  hasPeerThisAddress ? "hasPeerThisAddress" : "peerHasNoThis",
			  hasSendingIdentity ? "hasSendingIdentity"
								 : "doesNotHaveSendingIdentity",
			  host->_InternalCanSendMorePackets() ? "CanSendMore"
												  : "NoSpaceAvailableToSend");

	if (hasPeerThisAddress == false) {
		if (hasSendingIdentity) {
			if (host->_InternalCanSendMorePackets()) {
				// TODO: do it only once every 50 ms or so
				uint8_t *packetPackingPtr =
					(uint8_t *)host->_InternalGetNextPacketDataPointer();
				memcpy(packetPackingPtr, &sendingIdentity, 4);
				host->_InternalFinishSendingPacket(4, &ip4);
				LOG_DEBUG("Sending initial udp packet "
						  "||||||||||||||||||||||||||||||||");
				LOG_DEBUG(" first, last: %i %i", host->firstFilledSendPacketId,
						  host->lastFilledSendPacketId);
			} else {
				//
	LOG_DEBUG("333333333333333333333333333333333333333");
			}
		} else {
			LOG_DEBUG("2222222222222222222222222222222222222");
		}
	} else {
		LOG_DEBUG("111111111111111111111111111111111111111");
	}
	LOG_DEBUG(" first, last: %i %i", host->firstFilledSendPacketId,
			  host->lastFilledSendPacketId);
	if (hasRemoteAddress == false) {
		if (udpSendFrames.size() > 50) {
			// TODO: reconsider dropping all unreliable packets befor
			// 		 establishing upd endpoint
			udpSendFrames.clear();
		}
		return;
	}
	tmpUdpPacketCollection.clear();
	tmpUdpPacketCollectionSizes.clear();
	while (host->_InternalCanSendMorePackets() &&
		   udpSendFrames.empty() == false) {
		uint32_t totalSize = 0;

		for (auto it = udpSendFrames.rbegin(); it != udpSendFrames.rend();
			 ++it) {
			if (totalSize == 0 ||
				((totalSize + it->first) <= maxUnreliablePacketSize)) {
				totalSize += it->first;
				tmpUdpPacketCollectionSizes.push_back(totalSize);
				if (totalSize >= maxUnreliablePacketSize) {
					break;
				}
			}
		}

		// TODO: adjust it for encryption
		uint8_t *const _packetPackingPtr =
			(uint8_t *)host->_InternalGetNextPacketDataPointer();
		uint8_t *packetPackingPtr = _packetPackingPtr;

		memcpy(packetPackingPtr, &sendingIdentity, 4);
		packetPackingPtr += 4;
		memcpy(packetPackingPtr, &sendingNewestPacketId, 4);
		++sendingNewestPacketId;
		packetPackingPtr += 4;

		for (auto it : tmpUdpPacketCollectionSizes) {
			auto p = udpSendFrames.find(it);
			// no check if p != .end() becaues in
			// tmpUdpPacketCollectionSizes should be only valid keys
			auto &f = p->second;
			memcpy(packetPackingPtr, f.header, f.headerSize);
			packetPackingPtr += f.headerSize;
			memcpy(packetPackingPtr, f.dataWithoutHeader.data(),
				   f.dataWithoutHeader.size());
			packetPackingPtr += f.dataWithoutHeader.size();
			udpSendFrames.erase(it);
		}

		if (SSL) {
			// TODO: in place encryption, add sequence number, add MAC,
			// adjust totalSize: totalSize += encryption_overhead;
		}

		host->_InternalFinishSendingPacket(totalSize, &ip4);

		tmpUdpPacketCollection.clear();
		tmpUdpPacketCollectionSizes.clear();
	}
	*/
}

void Peer::_InternalDisconnect()
{
	icon7::uS::tcp::Peer::_InternalClearInternalDataOnClose();
	// TODO: check if do nothing is good
}

void Peer::_InternalClearInternalDataOnClose()
{
	icon7::uS::tcp::Peer::_InternalClearInternalDataOnClose();
	// TODO: check if do nothing is good
}

bool Peer::_InternalHasQueuedSends() const
{
	if (udpSendFrames.empty() == false) {
		return true;
	}
	if (hasPeerThisAddress == false) {
		return true;
	}
	return icon7::uS::tcp::Peer::_InternalHasQueuedSends();
}

void Peer::_InternalOnUdpPacket(void *data, uint32_t bytes)
{
	LOG_DEBUG(" . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .");
	bitscpp::ByteReader<true> reader((uint8_t *)data, 0, bytes);

	LOG_FATAL("Not reimplemented with ByteBuffer");
	throw "Not reimplemented with ByteBuffe: "
		  "icon7::uS::tcpudp::Peer::_InternalOnUdpPacket";

	/*
// 	if (bytes == 0) {
// 		hasRemoteAddress = true;
// 		std::vector<uint8_t> data;
// 		data.resize(1);
// 		data[0] = 0x81;
// 		if (SSL) {
// 			memcpy(data.data() + 5, sendingKey, 32);
// 		}
// 		SendLocalThread(std::move(data),
// 						FLAG_UNRELIABLE | FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
// 	}

	uint32_t packetId = 0;
	reader.op(packetId);

	if (receivingNewestPacketId >= packetId) {
		if (((receivingNewestPacketId & 0x40000000) == 0x40000000) &&
			((packetId & 0x40000000) == 0)) {
			// TODO: restarting receivingNewestPacketId
		} else {
			return;
		}
	} else if (packetId - receivingNewestPacketId > 0x10000000) {
		// dropping this packet: assuming that the greater value of packetId is
		// in before rounding around
		return;
	}

	if (packetId < receivingNewestPacketId) {
		// TODO: increment hidden nonce
	}

	if (SSL) {
		// TODO: decrypt SSL
		// if decrypt fails ignore packet
		// revert incrementation of hidden nonce
	}

	{
		receivingNewestPacketId = packetId;
		udpFrameDecoder.Restart();
		udpFrameDecoder.PushData(
			(uint8_t *)data, bytes, _Internal_static_OnPacket,
			// 			[](std::vector<uint8_t> &buffer, uint32_t headerSize,
			// void *_peer) { 				Peer *peer = (Peer *)_peer;
			// 				peer->host->GetRpcEnvironment()->OnReceive(
			// 					peer, buffer, headerSize, FLAG_UNRELIABLE);
			// 			},
			this);
	}
	*/
}

void Peer::_InternalOnPacketWithControllSequenceBackend(ByteBuffer &buffer,
														uint32_t headerSize)
{
	ByteReader reader(buffer, headerSize);
	uint8_t vectorCall = 0;
	reader.op(vectorCall);
	switch (vectorCall) {
	case 0x80:
		if (buffer.size() < 5 + (SSL ? 32 : 0)) {
			// TODO: error, invalid packet size
			LOG_ERROR("Invalid 0x80 packet size");
		}
		reader.op((uint8_t *)&sendingIdentity, 4);
		if (SSL) {
			reader.op(receivingKey, 32);
		}
		hasSendingIdentity = true;
		break;
	case 0x81:
		hasPeerThisAddress = true;
		break;
	default:
		icon7::uS::tcp::Peer::_InternalOnPacketWithControllSequenceBackend(
			buffer, headerSize);
	}
}

void Peer::_InternalOnOpenFinish()
{
	hasPeerThisAddress = false;
	hasRemoteAddress = false;
	hasSendingIdentity = false;
	receivingIdentity =
		((Host *)host)
			->GenerateNewReceivingIdentityForPeer(
				std::dynamic_pointer_cast<Peer>(shared_from_this()));
	if (isClient) {
		hasRemoteAddress = true;
	} else {
		hasPeerThisAddress = true;
	}
	// 	std::vector<uint8_t> data;
	// 	data.resize(5 + (SSL ? 32 : 0));
	// 	data[0] = 0x80;
	// 	memcpy(data.data() + 1, &receivingIdentity, 4);
	// 	if (SSL) {
	// 		memcpy(data.data() + 5, sendingKey, 32);
	// 	}
	// 	SendLocalThread(std::move(data),
	// 					FLAG_RELIABLE | FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
}

} // namespace tcpudp
} // namespace uS
} // namespace icon7
