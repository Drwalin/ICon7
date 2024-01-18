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

protected:
	// push packets to udpSendFrames
	// or
	// override  _InternalPopQueuedSendsFromAsync() and move directly to
	// udpSendFrames from sendQueue
	virtual bool _InternalSend(SendFrameStruct &dataFrame,
							   bool hasMore) override;
	virtual void _InternalDisconnect() override;

	virtual void _InternalClearInternalDataOnClose() override;

	virtual bool _InternalHasQueuedSends() const override;

	friend class HostUStcp;

protected:
	IPProto ipVersion;
	Host::IpAddress ipAddress;
	std::vector<SendFrameStruct> udpSendFrames;
};
} // namespace tcpudp
} // namespace uS
} // namespace icon7

#endif
