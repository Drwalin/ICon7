// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
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
	host->_InternalInsertPeerToFlush(peer.get());
}

void ExecuteRPC::Execute()
{
	messageConverter->Call(peer.get(), reader, flags, returnId);
}

void ExecuteConnect::Execute() { host->_InternalConnect(*this); }

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
		onListen.~CommandHandle<ExecuteBooleanOnHost>();
		onListen._com = nullptr;
	} else {
		host->_InternalListen(address, ipProto, port, onListen);
		if (queue) {
			queue->EnqueueCommand(std::move(onListen));
		} else {
			onListen.Execute();
		}
	}
}

void ExecuteDisconnect::Execute() { peer->_InternalDisconnect(); }

} // namespace internal
} // namespace commands
} // namespace icon7
