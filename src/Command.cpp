// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/CommandExecutionQueue.hpp"

#include "../include/icon7/Command.hpp"

namespace icon7
{
Command::~Command() {}

namespace commands
{
namespace internal
{

void ExecuteAddPeerToFlush::Execute()
{
	peer.GetLoop()->_InternalInsertPeerToFlush(peer);
}

void ExecuteRPC::Execute()
{
	assert(messageConverter != nullptr);
	messageConverter->DeserializeAndExecute(queue, peer, reader, flags, returnId);
}

void ExecuteConnect::Execute() {
	assert(host != nullptr);
	host->_InternalConnect(*this); }

void ExecuteListen::Execute()
{
	if (onListen.IsValid() == false) {
		class DummyOnListen final : public ExecuteBooleanOnHost
		{
		public:
			DummyOnListen() {}
			virtual ~DummyOnListen() {}
			virtual void Execute() override {}
		};
		onListen = CommandHandle<DummyOnListen>::Create();
		onListen->host = host;
		host->_InternalListen(address, ipProto, port, onListen);
		onListen.~CommandHandle();
	} else {
		host->_InternalListen(address, ipProto, port, onListen);
		if (queue) {
			queue->EnqueueCommand(std::move(onListen));
		} else {
			onListen.Execute();
		}
	}
}

void ExecuteDisconnect::Execute() { 
	if (peer) {
		if (PeerData *data = peer.GetLocalPeerData()) {
			data->_InternalDisconnect(); 
		}
	}
}

void ExecutePeerSendFrame::Execute()
{
	if (peer) {
		PeerData *data =peer.GetLocalPeerData();
		if (data) {
			data->Send(std::move(frame));
		}
	}
}

} // namespace internal
} // namespace commands
} // namespace icon7
