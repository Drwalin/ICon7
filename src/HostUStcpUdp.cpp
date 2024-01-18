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

#define ICON7_INCLUDE_CONCURRENT_QUEUE_CPP

#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/PeerUStcpUdp.hpp"

#include "../include/icon7/HostUStcpUdp.hpp"

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
}

Host::~Host() {}

void Host::_InternalDestroy() { icon7::uS::tcp::Host::_InternalDestroy(); }

void Host::_InternalListen(IPProto ipProto, uint16_t port,
						   commands::ExecuteBooleanOnHost &&com,
						   CommandExecutionQueue *queue)
{
	sendingBuffer = us_create_udp_packet_buffer();
	receivingBuffer = us_create_udp_packet_buffer();
}

void Host::_InternalSingleLoopIteration()
{
	icon7::uS::tcp::Host::_InternalSingleLoopIteration();
}

void Host::StopListening()
{
	icon7::uS::tcp::Host::StopListening();
	// TODO: correct this code, use future implementation for us_udp_socket_free
	// and us_udp_packet_buffer_free
	if (udpSocket) {
		us_poll_t *poll = (us_poll_t *)udpSocket;
		auto fd = us_poll_fd(poll);
		us_poll_stop(poll, loop);
		us_poll_free(poll, loop);
		bsd_close_socket(fd);

		free(receivingBuffer);
		free(sendingBuffer);
		udpSocket = nullptr;
		receivingBuffer = nullptr;
		sendingBuffer = nullptr;
	}
}

void Host::_InternalConnect(commands::ExecuteConnect &connectCommand)
{
	// TODO: init handshake messages
}

std::shared_ptr<icon7::uS::tcp::Peer> Host::MakePeer(us_socket_t *socket)
{
	return std::make_shared<icon7::uS::tcpudp::Peer>(this, socket);
}

} // namespace tcpudp
} // namespace uS
} // namespace icon7
