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

#include <thread>

#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/Command.hpp"

#include "../include/icon7/PeerUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcp
{
Peer::Peer(uS::tcp::Host *host, us_socket_t *socket) : icon7::Peer(host)
{
	this->socket = socket;
	SSL = host->SSL;
}

Peer::~Peer() { socket = nullptr; }

bool Peer::_InternalSend(SendFrameStruct &f, bool hasMore)
{
	if (f.headerBytesSent < f.headerSize) {
		f.headerBytesSent +=
			us_socket_write(SSL, socket, (char *)(f.header + f.headerBytesSent),
							f.headerSize - f.headerBytesSent,
							(f.dataWithoutHeader.size() ? 1 : 0) || hasMore);
		if (f.headerBytesSent < f.headerSize) {
			return false;
		}
	}
	if (f.bytesSent < f.dataWithoutHeader.size()) {
		f.bytesSent += us_socket_write(
			SSL, socket, (char *)(f.dataWithoutHeader.data() + f.bytesSent),
			f.dataWithoutHeader.size() - f.bytesSent, hasMore);
	}
	if (f.bytesSent < f.dataWithoutHeader.size()) {
		return false;
	}
	return true;
}

void Peer::_InternalDisconnect() { us_socket_shutdown(SSL, socket); }

void Peer::_InternalClearInternalDataOnClose()
{
	icon7::Peer::_InternalClearInternalDataOnClose();
	socket = nullptr;
	SSL = 0;
}
} // namespace tcp
} // namespace uS
} // namespace icon7
