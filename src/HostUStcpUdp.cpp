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

#ifdef ICON7_ENBALE_EXPERIMENTAL_UDP
#include <cstring>

#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/Debug.hpp"

#include "../include/icon7/HostUStcpUdp.hpp"
#include "../include/icon7/PeerUStcpUdp.hpp"

extern "C" void bsd_close_socket(LIBUS_SOCKET_DESCRIPTOR fd);

namespace icon7
{
namespace uS
{
namespace tcpudp
{
Host::Host()
{
	udpSocket = nullptr;
	receivingBuffer = nullptr;
	sendingBuffer = nullptr;
	firstFilledSendPacketId = 0;
	lastFilledSendPacketId = 0;
}

Host::~Host() {}

void Host::_InternalDestroy() { icon7::uS::tcp::Host::_InternalDestroy(); }

void Host::_InternalListen(const std::string &address, IPProto ipProto,
						   uint16_t port,
						   CommandHandle<commands::ExecuteBooleanOnHost> &com)
{
	const char *proto = "127.0.0.1";
	if (ipProto == IPv6) {
		proto = "::1";
	}
	if (address.size() != 0) {
		proto = address.c_str();
	}
	sendingBuffer = us_create_udp_packet_buffer();
	receivingBuffer = us_create_udp_packet_buffer();
	udpSocket =
		us_create_udp_socket(loop, receivingBuffer, _InternalOnReceiveUdpPacket,
							 _InternalOnDrainUdpSocket, proto, port, this);
	if (udpSocket == nullptr) {
		com->result = false;
		return;
	}

	icon7::uS::tcp::Host::_InternalListen(address, ipProto, port, com);

	if (com->result == false) {
		us_close_and_free_udp_socket(udpSocket, 0);
		free(receivingBuffer);
		free(sendingBuffer);
		udpSocket = nullptr;
		receivingBuffer = nullptr;
		sendingBuffer = nullptr;
	}
}

void Host::_InternalSingleLoopIteration(bool forceExecution)
{
	_InternalPerformSend();
	icon7::uS::tcp::Host::_InternalSingleLoopIteration(forceExecution);
	_InternalPerformSend();
}

bool Host::_InternalCanSendMorePackets()
{
	return (lastFilledSendPacketId - firstFilledSendPacketId) < 1000;
}

void *Host::_InternalGetNextPacketDataPointer()
{
	int32_t currentSendPacketId;
	if (firstFilledSendPacketId > 0) {
		currentSendPacketId = firstFilledSendPacketId - 1;
	} else {
		currentSendPacketId = lastFilledSendPacketId;
	}
	if (currentSendPacketId >= 0) {
		return (void *)us_udp_packet_buffer_payload(sendingBuffer,
													currentSendPacketId);
	}
	return nullptr;
}

void Host::_InternalFinishSendingPacket(uint32_t bytesCount, void *address)
{
	int32_t currentSendPacketId;
	if (firstFilledSendPacketId > 0) {
		currentSendPacketId = firstFilledSendPacketId - 1;
	} else {
		currentSendPacketId = lastFilledSendPacketId;
	}
	us_udp_buffer_set_packet_payload(sendingBuffer, currentSendPacketId,
									 bytesCount, (void *)"", 0, address);
	if (currentSendPacketId == lastFilledSendPacketId) {
		++lastFilledSendPacketId;
	} else {
		--firstFilledSendPacketId;
	}
	currentSendPacketId = -1;
	if (lastFilledSendPacketId - firstFilledSendPacketId > 50) {
		_InternalPerformSend();
	}
}

void Host::_InternalPerformSend()
{
	if (lastFilledSendPacketId - firstFilledSendPacketId == 0) {
		return;
	}

	LOG_DEBUG("Send udp ################################################");
	int sentPackets = us_udp_socket_send_range(udpSocket, sendingBuffer,
											   firstFilledSendPacketId,
											   lastFilledSendPacketId);
	if (sentPackets > 0) {
		firstFilledSendPacketId += sentPackets;
	}
}

void Host::_InternalOnReceiveUdpPacket(int receivedPacketsCount)
{
	for (int i = 0; i < receivedPacketsCount; ++i) {
		void *endpoint = us_udp_packet_buffer_peer(receivingBuffer, i);

		struct sockaddr_storage *addr = (struct sockaddr_storage *)endpoint;
		IPProto proto;
		if (addr->ss_family == AF_INET) {
			proto = IPv4;
		} else if (addr->ss_family == AF_INET6) {
			proto = IPv6;
		} else {
			// invalid packet received: dropping
			continue;
		}

		uint8_t *payload =
			(uint8_t *)us_udp_packet_buffer_payload(receivingBuffer, i);
		int bytes = us_udp_packet_buffer_payload_length(receivingBuffer, i);

		uint32_t peerIdentity = 0;
		memcpy(&peerIdentity, payload, 4);

		auto it = peerByReceivingIdentity.find(peerIdentity);
		if (it == peerByReceivingIdentity.end()) {
			// TODO: show error for invalid peer receiving identity
		} else {
			if (it->second->hasRemoteAddress == false) {
				if (proto == IPv4) {
					memcpy(&it->second->ip4, addr, sizeof(struct sockaddr_in));
				} else {
					memcpy(&it->second->ip4, addr, sizeof(struct sockaddr_in6));
				}
			}
			it->second->_InternalOnUdpPacket(payload + 4, bytes - 4);
		}
	}
}

void Host::_InternalOnReceiveUdpPacket(us_udp_socket_t *socket,
									   us_udp_packet_buffer_t *buffer,
									   int receivedPacketsCount)
{
	Host *host = *(Host **)us_udp_socket_user(socket);
	host->_InternalOnReceiveUdpPacket(receivedPacketsCount);
}

void Host::_InternalOnDrainUdpSocket(us_udp_socket_t *socket)
{
	Host *host = *(Host **)us_udp_socket_user(socket);
	host->_InternalOnDrainUdpSocket();
}

void Host::_InternalOnDrainUdpSocket() { _InternalPerformSend(); }

void Host::StopListening()
{
	icon7::uS::tcp::Host::StopListening();
	// TODO: correct this code, use future implementation for us_udp_socket_free
	// and us_udp_packet_buffer_free
	if (udpSocket) {
		us_close_and_free_udp_socket(udpSocket, 0);
		free(receivingBuffer);
		free(sendingBuffer);
		udpSocket = nullptr;
		receivingBuffer = nullptr;
		sendingBuffer = nullptr;
	}
}

std::shared_ptr<icon7::uS::tcp::Peer> Host::MakePeer(us_socket_t *socket)
{
	return std::make_shared<icon7::uS::tcpudp::Peer>(this, socket);
}

void Host::_Internal_on_open_Finish(std::shared_ptr<icon7::Peer> _peer)
{
	icon7::uS::tcp::Host::_Internal_on_open_Finish(_peer);
	Peer *peer = (Peer *)_peer.get();

	peer->_InternalOnOpenFinish();
}

uint32_t Host::GenerateNewReceivingIdentityForPeer(std::shared_ptr<Peer> peer)
{
	for (;; ++firstFreeReceivingIdentity) {
		if (firstFreeReceivingIdentity == 0) {
			continue;
		}
		if (peerByReceivingIdentity.count(firstFreeReceivingIdentity) == 0) {
			break;
		}
	}
	peerByReceivingIdentity[firstFreeReceivingIdentity] = peer;
	return firstFreeReceivingIdentity;
}

} // namespace tcpudp
} // namespace uS
} // namespace icon7
#endif
