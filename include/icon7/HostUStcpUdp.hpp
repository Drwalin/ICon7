// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifdef ICON7_ENBALE_EXPERIMENTAL_UDP
#ifndef ICON7_HOST_US_TCPUDP_HPP
#define ICON7_HOST_US_TCPUDP_HPP

#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "../uSockets/src/libusockets.h"

#include "FrameDecoder.hpp"

#include "HostUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcpudp
{
class Peer;

class Host : public icon7::uS::tcp::Host
{
public:
	Host();
	virtual ~Host();

	virtual void _InternalDestroy() override;

	virtual void _InternalListen(
		const std::string &address, IPProto ipProto, uint16_t port,
		CommandHandle<commands::ExecuteBooleanOnHost> &com) override;

	virtual void _InternalSingleLoopIteration(bool forceExecution) override;

	virtual void StopListening() override;

	bool _InternalCanSendMorePackets();
	void *_InternalGetNextPacketDataPointer();
	void _InternalFinishSendingPacket(uint32_t bytesCount, void *address);
	void _InternalPerformSend();

	virtual void
	_Internal_on_open_Finish(std::shared_ptr<icon7::Peer> peer) override;

	uint32_t GenerateNewReceivingIdentityForPeer(std::shared_ptr<Peer> peer);

	friend class Peer;

protected:
	static void _InternalOnReceiveUdpPacket(us_udp_socket_t *socket,
											us_udp_packet_buffer_t *buffer,
											int receivedPacketsCount);
	void _InternalOnReceiveUdpPacket(int receivedPacketsCount);

	static void _InternalOnDrainUdpSocket(us_udp_socket_t *socket);
	void _InternalOnDrainUdpSocket();

	virtual std::shared_ptr<icon7::uS::tcp::Peer>
	MakePeer(us_socket_t *socket) override;

protected:
	std::unordered_map<uint32_t, std::shared_ptr<Peer>> peerByReceivingIdentity;
	uint32_t firstFreeReceivingIdentity;

	us_udp_socket_t *udpSocket;
	us_udp_packet_buffer_t *receivingBuffer;
	us_udp_packet_buffer_t *sendingBuffer;

	FrameDecoder frameDecoder;

	uint32_t firstFilledSendPacketId, lastFilledSendPacketId;
};
} // namespace tcpudp
} // namespace uS
} // namespace icon7

#endif
#endif
