// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifdef ICON7_ENBALE_EXPERIMENTAL_UDP
#ifndef ICON7_PEER_US_TCPUDP_HPP
#define ICON7_PEER_US_TCPUDP_HPP

#include <map>
#include <vector>

#include "SendFrameStruct.hpp"
#include "HostUStcpUdp.hpp"

#include "PeerUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcpudp
{
class Peer : public icon7::uS::tcp::Peer
{
public:
	Peer(uS::tcpudp::Host *host, us_socket_t *socket);
	virtual ~Peer();

	const uint32_t maxUnreliablePacketSize;

protected:
	/* push packets to udpSendFrames
	 * or
	 * override  _InternalPopQueuedSendsFromAsync() and move directly to
	 * udpSendFrames from sendQueue
	 * if total frame size is larger than maxUnreliablePacketSize then packets
	 * are send alone in hopes that the internet won't drop it. User needs to
	 * take care of size of packets
	 */
	virtual bool _InternalSend(SendFrameStruct &dataFrame,
							   bool hasMore) override;
	virtual bool _InternalFlushQueuedSends() override;
	virtual void _InternalDisconnect() override;

	virtual void _InternalClearInternalDataOnClose() override;

	virtual bool _InternalHasQueuedSends() const override;

	void _InternalOnUdpPacket(void *data, uint32_t bytes);
	virtual void
	_InternalOnPacketWithControllSequenceBackend(ByteBuffer &buffer,
												 uint32_t headerSize) override;

	void _InternalOnOpenFinish();

	friend class Host;

protected:
	union {
		struct sockaddr_in ip4;
		struct sockaddr_in6 ip6;
	};
	std::multimap<uint32_t, SendFrameStruct> udpSendFrames;
	FrameDecoder udpFrameDecoder;

	uint32_t sendingIdentity;	// received from peer
	uint32_t receivingIdentity; // generatea from Host counter
	uint32_t receivingNewestPacketId;
	uint32_t sendingNewestPacketId;
	uint8_t sendingKey[32];
	uint8_t receivingKey[32];

	bool hasPeerThisAddress;
	bool hasRemoteAddress;
	bool hasSendingIdentity;

private:
	std::vector<SendFrameStruct> tmpUdpPacketCollection;
	std::vector<int> tmpUdpPacketCollectionSizes;
};
} // namespace tcpudp
} // namespace uS
} // namespace icon7

#endif
#endif
