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
PeerUStcp::PeerUStcp(Host *host, us_socket_t *socket) : Peer(host)
{
	this->socket = socket;
	SSL = ((HostUStcp *)host)->SSL;
}

PeerUStcp::~PeerUStcp() { socket = nullptr; }

bool PeerUStcp::_InternalSend(SendFrameStruct &f, bool hasMore)
{
	if (f.headerBytesSent < f.headerSize) {
		uint8_t *b = f.header;
		DEBUG("sending header bytes: %u   --   %2.2X %2.2X %2.2X %2.2X",
			  f.headerSize - f.headerBytesSent, b[0], b[1], b[2], b[3]);
		f.headerBytesSent +=
			us_socket_write(SSL, socket, (char *)(f.header + f.headerBytesSent),
							f.headerSize - f.headerBytesSent,
							(f.dataWithoutHeader.size() ? 1 : 0) || hasMore);
		if (f.headerBytesSent < f.headerSize) {
			DEBUG("FALSE");
			return false;
		}
	}
	DEBUG("");
	if (f.bytesSent < f.dataWithoutHeader.size()) {
		uint8_t *b = f.dataWithoutHeader.data();
		DEBUG("sending data bytes: %u   --   %2.2X %2.2X %2.2X %2.2X %2.2X",
			  f.dataWithoutHeader.size() - f.bytesSent, b[0], b[1], b[2], b[3],
			  b[4]);
		f.bytesSent += us_socket_write(
			SSL, socket, (char *)(f.dataWithoutHeader.data() + f.bytesSent),
			f.dataWithoutHeader.size() - f.bytesSent, hasMore);
	}
	if (f.bytesSent < f.dataWithoutHeader.size()) {
		DEBUG("FALSE");
		return false;
	}
	DEBUG("Sent: [header: %i] [body: %i]", f.headerSize,
		  f.dataWithoutHeader.size());
	DEBUG("TRUE");
	return true;
}

void PeerUStcp::_InternalDisconnect() { us_socket_shutdown(SSL, socket); }
} // namespace icon7
