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

#ifndef ICON7_PEER_US_TCP_HPP
#define ICON7_PEER_US_TCP_HPP

#include "../../uSockets/src/libusockets.h"

#include "Peer.hpp"

namespace icon7
{
class Host;

namespace uS
{
namespace tcp
{
class Host;

class Peer : public icon7::Peer
{
public:
	Peer(uS::tcp::Host *host, us_socket_t *socket);
	virtual ~Peer();

protected:
	virtual bool _InternalSend(SendFrameStruct &dataFrame,
							   bool hasMore) override;
	virtual void _InternalDisconnect() override;

	virtual void _InternalClearInternalDataOnClose() override;
	
	virtual bool _InternalHasBufferedSends() const override;
	
	virtual bool _InternalFlushBufferedSends(bool hasMore) override;

	friend class Host;

protected:
	us_socket_t *socket;
	int SSL;
	
	ByteBuffer writeBuffer;
	uint32_t writeBufferOffset = 0;
};
} // namespace tcp
} // namespace uS
} // namespace icon7

#endif
