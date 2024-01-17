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

#ifndef ICON7_PEER_USTCP_HPP
#define ICON7_PEER_USTCP_HPP

#include "../../uSockets/src/libusockets.h"

#include "Peer.hpp"

namespace icon7
{
class Host;
class HostUStcp;

class PeerUStcp : public Peer
{
public:
	PeerUStcp(Host *host, us_socket_t *socket);
	virtual ~PeerUStcp();

protected:
	virtual bool _InternalSend(SendFrameStruct &dataFrame,
							   bool hasMore) override;
	virtual void _InternalDisconnect() override;

	virtual void _InternalClearInternalDataOnClose() override;

	friend class HostUStcp;

protected:
	us_socket_t *socket;
	int SSL;
};
} // namespace icon7

#endif
