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

#ifndef ICON7_PEER_HPP
#define ICON7_PEER_HPP

#include <cinttypes>

#include <vector>
#include <atomic>
#include <memory>
#include <queue>

#include "Flags.hpp"
#include "ByteReader.hpp"
#include "ByteWriter.hpp"
#include "icon7/Command.hpp"

namespace icon7
{
class Host;

class Peer : public std::enable_shared_from_this<Peer>
{
public:
	virtual ~Peer();

	void Send(std::vector<uint8_t> &&dataWithoutHeader, Flags flags);
	void Disconnect();

	inline bool IsReadyToUse() const { return readyToUse; }
	inline bool IsDisconnecting() const { return disconnecting; }

	void SetOnDisconnect(void (*callback)(Peer *)) { onDisconnect = callback; }

public: // thread unsafe, safe only in hosts loop thread
	void _InternalOnData(uint8_t *data, uint32_t length);
	void _InternalOnDisconnect();
	void _InternalOnWritable();
	void _InternalOnTimeout();
	void _InternalOnLongTimeout();

	bool _InternalHasQueuedSends() const;

public:
	uint64_t userData;
	void *userPointer;

public:
	Host *const host;

protected:
	struct SendFrameStruct {
		std::vector<uint8_t> dataWithoutHeader;
		uint32_t bytesSent;
		Flags flags;
		uint8_t header[4];
		uint8_t headerBytesSent;
		uint8_t headerSize;

		SendFrameStruct(std::vector<uint8_t> &&dataWithoutHeader, Flags flags);
		SendFrameStruct();
	};

	void _InternalFlushQueuedSends();
	void _InternalPopQueuedSendsFromAsync();
	/*
	 * return true if successfully whole dataFrame has been sent.
	 * false otherwise.
	 */
	virtual bool _InternalSend(SendFrameStruct &dataFrame, bool hasMore) = 0;
	virtual void _InternalDisconnect() = 0;

	friend class HostUStcp;
	friend class commands::ExecuteDisconnect;
	
	std::atomic<bool> closed;

protected:
	Peer(Host *host);

	void SetReadyToUse();

protected:
#ifdef ICON7_PEER_CPP_INCLUDE_UNION_CONCURRENT_QUEUE
	using QueueType = moodycamel::ConcurrentQueue<SendFrameStruct>;
#endif

	union {
		void *sendQueue_variable_name_placeholder;
#ifdef ICON7_PEER_CPP_INCLUDE_UNION_CONCURRENT_QUEUE
		QueueType *sendQueue;
#endif
	};
	std::queue<SendFrameStruct> sendQueueLocal;
	std::atomic<uint32_t> sendingQueueSize;

	void (*onDisconnect)(Peer *);

	std::atomic<bool> readyToUse;
	std::atomic<bool> disconnecting;
	

	std::vector<uint8_t> receivingFrameBuffer;
	uint32_t receivingHeaderSize;
	uint32_t receivingFrameSize;
};
} // namespace icon7

#endif
