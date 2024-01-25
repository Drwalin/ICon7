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

#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/HostUStcp.hpp"

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

void Host::_InternalListen(IPProto ipProto, uint16_t port,
						   commands::ExecuteBooleanOnHost &com)
{
	const char *proto = "127.0.0.1";
	if (ipProto == IPv6) {
		proto = "::1";
	}
	sendingBuffer = us_create_udp_packet_buffer();
	receivingBuffer = us_create_udp_packet_buffer();
	udpSocket =
		us_create_udp_socket(loop, receivingBuffer, _InternalOnReceiveUdpPacket,
							 _InternalOnDrainUdpSocket, proto, port, this);
	if (udpSocket == nullptr) {
		com.result = false;
		return;
	}

	icon7::uS::tcp::Host::_InternalListen(ipProto, port, com);

	if (com.result == false) {
		us_close_and_free_udp_socket(udpSocket, 0);
		free(receivingBuffer);
		free(sendingBuffer);
		udpSocket = nullptr;
		receivingBuffer = nullptr;
		sendingBuffer = nullptr;
	}
}

void Host::_InternalSingleLoopIteration()
{
	_InternalPerformSend();
	icon7::uS::tcp::Host::_InternalSingleLoopIteration();
	_InternalPerformSend();
}

bool Host::_InternalCanSendMorePackets()
{
	return (lastFilledSendPacketId - firstFilledSendPacketId) < 1024;
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
	if (lastFilledSendPacketId - firstFilledSendPacketId > 300) {
		_InternalPerformSend();
	}
}

void Host::_InternalPerformSend()
{
	if (lastFilledSendPacketId - firstFilledSendPacketId == 0) {
		return;
	}

	int sentPackets = us_udp_socket_send_range(udpSocket, sendingBuffer,
											   firstFilledSendPacketId,
											   lastFilledSendPacketId);
	if (sentPackets > 0) {
		firstFilledSendPacketId += sentPackets;
	}
}

void Host::_InternalOnReceiveUdpPacket(int receivedPacketsCount)
{
	IpAddress address;
	for (int i = 0; i < receivedPacketsCount; ++i) {
		memset(&address, 0, sizeof(IpAddress));
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

		if (proto == IPv4) {
			address.proto = 4;
			struct sockaddr_in ip = *(struct sockaddr_in *)addr;
			address.port = ip.sin_port;
			memcpy(address.ip, &ip.sin_addr, 4);
		} else {
			address.proto = 6;
			struct sockaddr_in6 ip = *(struct sockaddr_in6 *)addr;
			address.port = ip.sin6_port;
			memcpy(address.ip, &ip.sin6_addr, 4);
		}

		void *payload =
			(void *)us_udp_packet_buffer_payload(receivingBuffer, i);
		int bytes = us_udp_packet_buffer_payload_length(receivingBuffer, i);

		auto it = peerAddresses.find(address);
		if (it == peerAddresses.end()) {
			// TODO: try to add this address to the correct peer
			if (bytes != 32) {
				// TODO: invalid payload for unknown address, dropping packet
				continue;
			}
		} else {
			it->second->_InternalOnUdpPacket(payload, bytes);
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

	if (peer->isClient) {
		peer->hasEstablishUdpEndpoint = true;
		peer->hasConnectionIdentifier = false;
		if (peer->SSL) {
			std::vector<uint8_t> data;
			data.resize(33);
			data[0] = 0x81;
			memcpy(data.data() + 1, peer->receivingKey, 32);
			peer->SendLocalThread(std::move(data),
								  FLAG_RELIABLE |
									  FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
		}
	} else {
		peer->hasEstablishUdpEndpoint = false;
		peer->hasConnectionIdentifier = true;
		std::vector<uint8_t> data;
		data.resize(33 + (peer->SSL ? 32 : 0));
		data[0] = 0x80;
		memcpy(data.data() + 1, peer->connectionIdentifier, 32);
		if (peer->SSL) {
			memcpy(data.data() + 33, peer->receivingKey, 32);
		}
		peer->SendLocalThread(std::move(data),
							  FLAG_RELIABLE | FLAGS_PROTOCOL_CONTROLL_SEQUENCE);
	}
}

} // namespace tcpudp
} // namespace uS
} // namespace icon7
