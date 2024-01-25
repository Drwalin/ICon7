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

#ifndef ICON7_PEER_US_TCPUDP_HPP
#define ICON7_PEER_US_TCPUDP_HPP

#include "SendFrameStruct.hpp"
#include "HostUStcpUdp.hpp"

#include "PeerUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcpudp
{
class Peer : public icon7::uS::tcp::Peer
{
public:
	Peer(uS::tcpudp::Host *host, us_socket_t *socket);
	virtual ~Peer();
	
	const uint32_t maxUnreliablePacketSize;

protected:
	/* push packets to udpSendFrames
	 * or
	 * override  _InternalPopQueuedSendsFromAsync() and move directly to
	 * udpSendFrames from sendQueue
	 * if total frame size is larger than maxUnreliablePacketSize then packets
	 * are send alone in hopes that the internet won't drop it. User needs to
	 * take care of size of packets
	 */
	virtual bool _InternalSend(SendFrameStruct &dataFrame,
							   bool hasMore) override;
	virtual void _InternalFlushQueuedSends() override;
	virtual void _InternalDisconnect() override;

	virtual void _InternalClearInternalDataOnClose() override;

	virtual bool _InternalHasQueuedSends() const override;
	
	void _InternalOnUdpPacket(void *data, uint32_t bytes);
	virtual void _InternalOnPacketWithControllSequenceBackend(
			std::vector<uint8_t> &buffer, uint32_t headerSize) override;

	friend class Host;

protected:
	IPProto ipVersion;
	Host::IpAddress ipAddress;
	union {
		struct sockaddr_in ip4;
		struct sockaddr_in ip6;
	};
	std::multimap<uint32_t, SendFrameStruct> udpSendFrames;
	FrameDecoder udpFrameDecoder;
	
	uint8_t connectionIdentifier[32];
	uint8_t sendingKey[32];
	uint8_t receivingKey[32];
	
	bool hasEstablishUdpEndpoint;
	bool hasConnectionIdentifier;
	
private:
	std::vector<SendFrameStruct> tmpUdpPacketCollection; 
	std::vector<int> tmpUdpPacketCollectionSizes; 
};
} // namespace tcpudp
} // namespace uS
} // namespace icon7

#endif
