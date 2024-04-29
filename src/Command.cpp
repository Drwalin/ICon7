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

#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/CommandExecutionQueue.hpp"

#include "../include/icon7/Command.hpp"

namespace icon7
{
	
concurrent::buckets_pool<192> globalPool(1024*16);
thread_local nonconcurrent::thread_local_pool<192, 128> tlsPool(&globalPool);

Command::~Command() { __m_next = nullptr; }

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
			DummyOnListen() = default;
			virtual ~DummyOnListen() = default;
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

void ExecuteDisconnect::Execute()
{
	if (peer->IsClosed() == false) {
		peer->_InternalDisconnect();
	} else {
	}
}

} // namespace internal
} // namespace commands
} // namespace icon7
