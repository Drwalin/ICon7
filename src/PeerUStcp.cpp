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

#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/Command.hpp"
#include "icon7/Debug.hpp"

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
	if (socket == nullptr || !IsReadyToUse() || IsDisconnecting() ||
		IsClosed()) {
		return false;
	}
	if (f.bytesSent < f.data.size()) {
		int b =
			us_socket_write(SSL, socket, (char *)(f.data.data() + f.bytesSent),
							f.data.size() - f.bytesSent, hasMore);
		if (b < 0) {
			LOG_ERROR("us_socket_write returned: %i", b);
		} else if (b > 0) {
			f.bytesSent += b;
		} else {
			// TODO: what to do here?
		}
	}
	if (f.bytesSent < f.data.size()) {
		return false;
	}
	return true;
}

void Peer::_InternalDisconnect()
{
	peerFlags |= BIT_DISCONNECTING;
	us_socket_close(SSL, socket, 0, nullptr);
}

void Peer::_InternalClearInternalDataOnClose()
{
	icon7::Peer::_InternalClearInternalDataOnClose();
	*(icon7::Peer **)us_socket_ext(SSL, socket) = nullptr;
	socket = nullptr;
	SSL = 0;
}

} // namespace tcp
} // namespace uS
} // namespace icon7
