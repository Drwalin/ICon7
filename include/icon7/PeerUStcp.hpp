// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
