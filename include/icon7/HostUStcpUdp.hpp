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

#ifndef ICON7_HOST_US_TCPUDP_HPP
#define ICON7_HOST_US_TCPUDP_HPP

#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "../uSockets/src/libusockets.h"

#ifdef ICON7_INCLUDE_CONCURRENT_QUEUE_CPP
#include "../concurrentqueue/concurrentqueue.h"
#endif

#include "FrameDecoder.hpp"

#include "HostUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcpudp
{
class Peer;

class Host : public icon7::uS::tcp::Host
{
public:
	Host();
	virtual ~Host();

	virtual void _InternalDestroy() override;

	virtual void _InternalListen(const std::string &address, IPProto ipProto,
								 uint16_t port,
								 commands::ExecuteBooleanOnHost &com) override;

	virtual void _InternalSingleLoopIteration() override;

	virtual void StopListening() override;

	bool _InternalCanSendMorePackets();
	void *_InternalGetNextPacketDataPointer();
	void _InternalFinishSendingPacket(uint32_t bytesCount, void *address);
	void _InternalPerformSend();

	virtual void
	_Internal_on_open_Finish(std::shared_ptr<icon7::Peer> peer) override;

	uint32_t GenerateNewReceivingIdentityForPeer(std::shared_ptr<Peer> peer);

	friend class Peer;

protected:
	static void _InternalOnReceiveUdpPacket(us_udp_socket_t *socket,
											us_udp_packet_buffer_t *buffer,
											int receivedPacketsCount);
	void _InternalOnReceiveUdpPacket(int receivedPacketsCount);

	static void _InternalOnDrainUdpSocket(us_udp_socket_t *socket);
	void _InternalOnDrainUdpSocket();

	virtual std::shared_ptr<icon7::uS::tcp::Peer>
	MakePeer(us_socket_t *socket) override;

protected:
	std::unordered_map<uint32_t, std::shared_ptr<Peer>> peerByReceivingIdentity;
	uint32_t firstFreeReceivingIdentity;

	us_udp_socket_t *udpSocket;
	us_udp_packet_buffer_t *receivingBuffer;
	us_udp_packet_buffer_t *sendingBuffer;

	FrameDecoder frameDecoder;

	uint32_t firstFilledSendPacketId, lastFilledSendPacketId;
};
} // namespace tcpudp
} // namespace uS
} // namespace icon7

#endif
