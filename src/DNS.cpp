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

#include "../include/icon7/DNS.hpp"

namespace icon7
{

AddressInfo::AddressInfo()
{
	addr = nullptr;
}

AddressInfo::~AddressInfo()
{
	Clear();
}

void AddressInfo::Clear()
{
	if (addr) {
		free(addr);
		addr = nullptr;
	}
}
	
void AddressInfo::Populate(std::string addres, uint16_t port, IPProto proto)
{
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
	if (proto == IPv4) {
		hints.ai_family = AF_INET;
	} else {
		hints.ai_family = AF_INET6;
	}

    char port_string[16];
    snprintf(port_string, 16, "%d", port);

    if (getaddrinfo(host, port_string, &hints, &result)) {
        return LIBUS_SOCKET_ERROR;
    }

    LIBUS_SOCKET_DESCRIPTOR listenFd = LIBUS_SOCKET_ERROR;
    struct addrinfo *listenAddr;
    for (struct addrinfo *a = result; a && listenFd == LIBUS_SOCKET_ERROR; a = a->ai_next) {
        if (a->ai_family == AF_INET6) {
            listenFd = bsd_create_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            listenAddr = a;
        }
    }

    for (struct addrinfo *a = result; a && listenFd == LIBUS_SOCKET_ERROR; a = a->ai_next) {
        if (a->ai_family == AF_INET) {
            listenFd = bsd_create_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            listenAddr = a;
        }
    }

    if (listenFd == LIBUS_SOCKET_ERROR) {
        freeaddrinfo(result);
        return LIBUS_SOCKET_ERROR;
    }
}

bool AddressInfo::CopyAddressTo(void *ptr)
{
	if (addr) {
		
		
		return true;
	}
	return false;
}
}
