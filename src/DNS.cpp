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
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "../uSockets/src/internal/networking/bsd.h"

#include "../include/icon7/DNS.hpp"

namespace icon7
{

AddressInfo::AddressInfo() { proto = IPinvalid; }

AddressInfo::~AddressInfo() { Clear(); }

void AddressInfo::Clear() { proto = IPinvalid; }

struct sockaddr_storage *AddressInfo::Address()
{
	return (struct sockaddr_storage *)addressStorage;
}

bool AddressInfo::Populate(const std::string address, const uint16_t port,
						   const IPProto proto)
{
	Clear();
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_flags = AI_PASSIVE;
	if (proto == IPv4) {
		hints.ai_family = AF_INET;
	} else {
		hints.ai_family = AF_INET6;
	}

	char portString[16];
	snprintf(portString, 16, "%d", port);

	if (getaddrinfo(address.c_str(), portString, &hints, &result)) {
		return false;
	}

	struct addrinfo *addr = nullptr;
	if (proto == IPv6) {
		for (struct addrinfo *a = result; a && addr == nullptr;
			 a = a->ai_next) {
			if (a->ai_family == AF_INET6) {
				addr = a;
				this->proto = proto;
			}
		}
	} else if (proto == IPv4) {
		for (struct addrinfo *a = result; a && addr == nullptr;
			 a = a->ai_next) {
			if (a->ai_family == AF_INET) {
				addr = a;
				this->proto = proto;
			}
		}
	}

	if (proto == IPv4) {
		memcpy(Address(), addr, ADDRESS4_STORAGE_SIZE);
	} else if (proto == IPv6) {
		memcpy(Address(), addr, ADDRESS6_STORAGE_SIZE);
	}

	freeaddrinfo(result);

	if (proto == IPinvalid) {
		return false;
	}

	return true;
}

bool AddressInfo::CopyAddressTo(void *ptr)
{
	if (proto == IPinvalid) {
		return false;
	} else if (proto == IPv4) {
		memcpy(ptr, Address(), ADDRESS4_STORAGE_SIZE);
	} else if (proto == IPv6) {
		memcpy(ptr, Address(), ADDRESS6_STORAGE_SIZE);
	}
	return true;
}
} // namespace icon7
