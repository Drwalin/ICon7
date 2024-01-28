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

#include <algorithm>
#include <memory>

#include "../include/icon7/HostUStcpUdp.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
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
	hasPeerEstablishThisEndpoint = false;
	hasRemoteAddress = false;
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
		udpSendFrames.insert({bytes, std::move(dataFrame)});
		return true;
	} else {
		return icon7::uS::tcp::Peer::_InternalSend(dataFrame, hasMore);
	}
}

void Peer::_InternalFlushQueuedSends()
{
	icon7::uS::tcp::Peer::_InternalFlushQueuedSends();
	if (hasPeerEstablishThisEndpoint == false) {
		// TODO: send udp packets with connection identity until receiving
		// 		 controll sequence with vector call 0x82
	} else {
		if (hasRemoteAddress == false) {
			if (udpSendFrames.size() > 50) {
				// TODO: reconsider dropping all unreliable packets befor
				// 		 establishing upd endpoint
				udpSendFrames.clear();
			}
			return;
		}
		Host *host = (Host *)(this->host);
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
			uint8_t *packetPackingPtr =
				(uint8_t *)host->_InternalGetNextPacketDataPointer();

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
				// TODO: encrypt SSL, adjust totalSize
				// totalSize += encryption_overhead;
			}

			host->_InternalFinishSendingPacket(totalSize, &ip4);

			tmpUdpPacketCollection.clear();
			tmpUdpPacketCollectionSizes.clear();
		}
	}
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
	if (hasPeerEstablishThisEndpoint == false) {
		return true;
	}
	return icon7::uS::tcp::Peer::_InternalHasQueuedSends();
}

void Peer::_InternalOnUdpPacket(void *data, uint32_t bytes)
{
	if (SSL) {
		// TODO: decrypt SSL
	}

	udpFrameDecoder.Restart();
	udpFrameDecoder.PushData(
		(uint8_t *)data, bytes,
		[](std::vector<uint8_t> &buffer, uint32_t headerSize, void *_peer) {
			Peer *peer = (Peer *)_peer;
			peer->host->GetRpcEnvironment()->OnReceive(peer, buffer, headerSize,
													   FLAG_UNRELIABLE);
		},
		this);
}

void Peer::_InternalOnPacketWithControllSequenceBackend(
	std::vector<uint8_t> &buffer, uint32_t headerSize)
{
	ByteReader reader(buffer, headerSize);
	uint8_t vectorCall = 0;
	reader.op(vectorCall);
	switch (vectorCall) {
	case 0x80:
		if (isClient) {
			reader.op(connectionIdentifier, 32);
			hasConnectionIdentifier = true;
			if (SSL) {
				if (reader.has_bytes_to_read(32)) {
					reader.op(receivingKey, 32);
				} else {
					// TODO: report error in establishing decryption key
				}
			}
		} else {
			// TODO: report error, server received what server should send
		}
		break;
	case 0x81:
		if (isClient == false) {
			if (SSL) {
				if (reader.has_bytes_to_read(32)) {
					reader.op(receivingKey, 32);
				} else {
					// TODO: report error in establishing decryption key
				}
			} else {
				// TODO: report this vector call is used only for SSL encrypted
				// connections
			}
		} else {
			// TODO: report error, client received what client should send
		}
		break;
	case 0x82:
		if (isClient == false) {
			hasPeerEstablishThisEndpoint = false;
		} else {
			// TODO: report error, client received what client should send
		}
		break;
	default:
		icon7::uS::tcp::Peer::_InternalOnPacketWithControllSequenceBackend(
			buffer, headerSize);
	}
}

void Peer::_InternalOnOpenFinish()
{
	hasPeerEstablishThisEndpoint = false;
	hasRemoteAddress = false;
	hasReceivingIdentity = false;
	receivingIdentity =
		((Host *)host)
			->GenerateNewReceivingIdentityForPeer(
				std::dynamic_pointer_cast<Peer>(shared_from_this()));
	if (isClient) {
		hasRemoteAddress = true;
		if (SSL) {
			std::vector<uint8_t> data;
			data.resize(33);
			data[0] = 0x81;
			memcpy(data.data() + 1, sendingKey, 32);
			SendLocalThread(std::move(data),
							FLAG_RELIABLE | FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
		}
	} else {
		hasPeerEstablishThisEndpoint = true;

		std::vector<uint8_t> data;
		data.resize(33 + (SSL ? 32 : 0));
		data[0] = 0x80;
		memcpy(data.data() + 1, connectionIdentifier, 32);
		if (SSL) {
			memcpy(data.data() + 33, sendingKey, 32);
		}
		SendLocalThread(std::move(data),
						FLAG_RELIABLE | FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
	}
}
} // namespace tcpudp
} // namespace uS
} // namespace icon7
