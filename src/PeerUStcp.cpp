// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <algorithm>

#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/SendFrameStruct.hpp"

#include "../include/icon7/PeerUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcp
{
Peer::Peer(uS::tcp::Host *host, us_socket_t *socket) : icon7::Peer(host)
{
	writeBuffer.Init(4000);
	this->socket = socket;
	SSL = host->SSL;
}

Peer::~Peer()
{
	if (socket != nullptr) {
		socket = nullptr;
		LOG_FATAL("on ~Peer(): socket != nullptr");
	}
}

bool Peer::_InternalSend(SendFrameStruct &f, bool hasMore)
{
	if (socket == nullptr) {
		return false;
	}

	if (writeBuffer.size() == writeBuffer.capacity()) {
		bool b = _InternalFlushBufferedSends(true);
		if (b == false) {
			return false;
		}
	}

	if (f.bytesSent < f.data.size()) {
		uint32_t bytesToWrite = f.data.size() - f.bytesSent;
		uint8_t *ptr = f.data.data() + f.bytesSent;

		if (writeBuffer.size() == writeBufferOffset &&
			(bytesToWrite > 500 || hasMore == false)) {
			if (f.bytesSent < f.data.size()) {
				int b = us_socket_write(SSL, socket,
										(char *)(f.data.data() + f.bytesSent),
										f.data.size() - f.bytesSent, hasMore);
				if (b < 0) {
					LOG_ERROR("us_socket_write returned: %i", b);
				} else if (b > 0) {
					f.bytesSent += b;
				} else {
					// TODO: what to do here?
					return false;
				}
			}
			if (f.bytesSent < f.data.size()) {
				return false;
			}
			return true;
		} else if (writeBuffer.size() + bytesToWrite <=
				   writeBuffer.capacity()) {
			writeBuffer.append(ptr, bytesToWrite);
			f.bytesSent += bytesToWrite;
			if (hasMore == false ||
				writeBuffer.size() == writeBuffer.capacity()) {
				return _InternalFlushBufferedSends(hasMore);
			}
			return true;
		} else {
			bytesToWrite = std::min<uint32_t>(
				bytesToWrite, writeBuffer.capacity() - writeBuffer.size());
			writeBuffer.append(ptr, bytesToWrite);
			f.bytesSent += bytesToWrite;
			bool canWriteMore = _InternalFlushBufferedSends(true);
			if (canWriteMore == false) {
				return false;
			} else {
				return _InternalSend(f, hasMore);
			}
		}
	} else {
		return true;
	}

	return false;
}

void Peer::_InternalDisconnect()
{
	peerFlags |= BIT_DISCONNECTING;
	if (socket) {
		us_socket_close(SSL, socket, 0, nullptr);
	} else {
		peerFlags |= BIT_CLOSED;
	}
}

void Peer::_InternalClearInternalDataOnClose()
{
	icon7::Peer::_InternalClearInternalDataOnClose();
	*(icon7::Peer **)us_socket_ext(SSL, socket) = nullptr;
	socket = nullptr;
	SSL = 0;
}

bool Peer::_InternalHasBufferedSends() const
{
	return writeBufferOffset < writeBuffer.size();
}

bool Peer::_InternalFlushBufferedSends(bool hasMore)
{
	int32_t bytesToSend = writeBuffer.size() - writeBufferOffset;
	if (bytesToSend > 0) {
		uint8_t *ptr = writeBuffer.data() + writeBufferOffset;
		int b = us_socket_write(SSL, socket, (char *)ptr, bytesToSend, hasMore);
		if (b < 0) {
			// TODO: check, maybe some ot this means cork?
			LOG_ERROR("us_socket_write returned: %i", b);
		} else if (b > 0) {
			writeBufferOffset += b;
			if (writeBuffer.size() == writeBufferOffset) {
				writeBuffer.clear();
				writeBufferOffset = 0;
				return true;
			} else {
				return false;
			}
		} else {
			// TODO: what to do here?
		}
	}
	return false;
}
} // namespace tcp
} // namespace uS
} // namespace icon7
