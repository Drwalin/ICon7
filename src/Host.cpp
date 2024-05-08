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

#include <chrono>
#include <thread>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Debug.hpp"

#include "../include/icon7/Host.hpp"

namespace icon7
{
Host::Host()
{
	userData = 0;
	userPointer = nullptr;
	onConnect = nullptr;
	onDisconnect = nullptr;
	asyncRunnerFlags = 0;
	rpcEnvironment = nullptr;
}

Host::~Host()
{
	WaitStopRunning();
	asyncRunner.join();
	if (rpcEnvironment) {
		rpcEnvironment->host = this;
	}
}

void Host::WaitStopRunning()
{
	QueueStopRunning();
	while (IsRunningAsync()) {
		QueueStopRunning();
		WakeUp();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Host::_InternalDestroy()
{
	StopListening();
	DisconnectAllAsync();
	QueueStopRunning();

	do {
		WakeUp();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	} while (IsRunningAsync());

	commandQueue.Execute(128);
	while (commandQueue.HasAny()) {
		commandQueue.Execute(128);
	}
}

void Host::DisconnectAllAsync()
{
	class CommandDisconnectAll final : public commands::ExecuteOnHost
	{
	public:
		CommandDisconnectAll() {}
		virtual ~CommandDisconnectAll() {}
		virtual void Execute() override { host->DisconnectAll(); }
	};
	auto com = CommandHandle<CommandDisconnectAll>::Create();
	com->host = this;
	EnqueueCommand(std::move(com));
}

void Host::DisconnectAll()
{
	std::vector<std::shared_ptr<Peer>> copy(peers.begin(), peers.end());
	for (auto &p : copy) {
		p->_InternalDisconnect();
	}
}

concurrent::future<std::shared_ptr<Peer>>
Host::ConnectPromise(std::string address, uint16_t port)
{
	class CommandConnectPromise final : public commands::ExecuteOnPeer
	{
	public:
		CommandConnectPromise() {}
		virtual ~CommandConnectPromise() {}

		concurrent::promise<std::shared_ptr<Peer>> promise;

		virtual void Execute() override
		{
			if (peer != nullptr) {
				promise.set_value(peer->shared_from_this());
			} else {
				promise.set_value(nullptr);
			}
		}
	};

	auto onConnected = CommandHandle<CommandConnectPromise>::Create();
	concurrent::future<std::shared_ptr<Peer>> future =
		onConnected->promise.get_future();
	Connect(address, port, std::move(onConnected), nullptr);
	return future;
}

void Host::_InternalConnect_Finish(commands::internal::ExecuteConnect &com)
{
	if (com.onConnected->peer.get() != nullptr) {
		peers.insert(com.onConnected->peer);
	}

	if (com.executionQueue) {
		com.executionQueue->EnqueueCommand(std::move(com.onConnected));
	} else {
		com.onConnected.Execute();
	}
}

void Host::_Internal_on_open_Finish(std::shared_ptr<Peer> peer)
{
	peers.insert(peer);

	if (onConnect) {
		onConnect(peer.get());
	}
	peer->SetReadyToUse();
}

void Host::_Internal_on_close_Finish(std::shared_ptr<Peer> peer)
{
	peer->peerFlags |= Peer::BIT_DISCONNECTING;
	peer->_InternalOnDisconnect();
	peer->_InternalClearInternalDataOnClose();
	peers.erase(peer);
}

concurrent::future<bool> Host::ListenOnPort(const std::string &address,
											uint16_t port, IPProto ipProto)
{
	class CommandListnePromise final : public commands::ExecuteBooleanOnHost
	{
	public:
		CommandListnePromise() {}
		virtual ~CommandListnePromise() {}

		concurrent::promise<bool> promise;

		virtual void Execute() override { promise.set_value(result); }
	};
	auto callback = CommandHandle<CommandListnePromise>::Create();
	callback->host = this;
	auto future = callback->promise.get_future();
	ListenOnPort(address, port, ipProto, std::move(callback), nullptr);
	return future;
}

void Host::ListenOnPort(
	const std::string &address, uint16_t port, IPProto ipProto,
	CommandHandle<commands::ExecuteBooleanOnHost> &&callback,
	CommandExecutionQueue *queue)
{
	if (callback.IsValid() == false) {
		LOG_FATAL("Used Host::ListenOnPort with empty callback");
		// TODO: maybe removethis dummy?
		class Dummy final : public commands::ExecuteBooleanOnHost
		{
		public:
			virtual ~Dummy() {}
			virtual void Execute() override {}
		};
		callback = CommandHandle<Dummy>();
	}
	auto com = CommandHandle<commands::internal::ExecuteListen>::Create();
	com->address = address;
	com->host = this;
	com->ipProto = ipProto;
	com->port = port;
	com->onListen = std::move(callback);
	com->queue = queue;

	EnqueueCommand(std::move(com));
}

void Host::Connect(std::string address, uint16_t port)
{
	Connect(address, port, {}, nullptr);
}

void Host::Connect(std::string address, uint16_t port,
				   CommandHandle<commands::ExecuteOnPeer> &&onConnected,
				   CommandExecutionQueue *queue)
{
	auto com = CommandHandle<commands::internal::ExecuteConnect>::Create();
	com->executionQueue = queue;
	com->port = port;
	com->address = address;
	com->onConnected = std::move(onConnected);
	com->host = this;
	EnqueueCommand(std::move(com));
}

void Host::EnqueueCommand(CommandHandle<Command> &&command)
{
	commandQueue.EnqueueCommand(std::move(command));
	WakeUp();
}

enum RunnerFlags : uint32_t { RUNNING = 1, QUEUE_STOP = 2 };

void Host::RunAsync()
{
	asyncRunnerFlags = 0;
	asyncRunner = std::thread(
		[](Host *host) {
			host->asyncRunnerFlags |= RUNNING;
			while (host->IsQueuedStopAsync() == false) {
				host->SingleLoopIteration();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			host->asyncRunnerFlags &= ~RUNNING;
		},
		this);
}

void Host::QueueStopRunning()
{
	asyncRunnerFlags |= QUEUE_STOP;
	if (IsRunningAsync()) {
		WakeUp();
	}
}

bool Host::IsQueuedStopAsync() { return asyncRunnerFlags & QUEUE_STOP; }

bool Host::IsRunningAsync() { return asyncRunnerFlags & RUNNING; }

void Host::_InternalSingleLoopIteration()
{
	commandQueue.Execute(128 * 1024);
	for (auto &p : peers) {
		if (p->_InternalHasBufferedSends() || p->_InternalHasQueuedSends()) {
			p->_InternalOnWritable();
		}
	}
	rpcEnvironment->CheckForTimeoutFunctionCalls(16);
}

void Host::InsertPeerToFlush(Peer *peer)
{
	// TODO: optimise this to remove passing command to host thread
	// i.e.: implement concurrent bit set of active peers
	/*
	commandQueue.GetThreadLocalBuffer()->EnqueueCommand<commands::internal::ExecuteAddPeerToFlush>(
		peer->shared_from_this(), this);
	*/
}

RPCEnvironment *Host::GetRpcEnvironment() { return rpcEnvironment; }

void Host::SetRpcEnvironment(RPCEnvironment *env)
{
	rpcEnvironment = env;
	if (rpcEnvironment->host != nullptr) {
		LOG_FATAL(
			"icon7::RPCEnvironment is used by multiple icon7::Host objects.");
	}
	rpcEnvironment->host = this;
}

void Host::_InternalInsertPeerToFlush(Peer *peer)
{
	// 	peersToFlush.insert(peer->shared_from_this());
}

void Host::ForEachPeer(void(func)(icon7::Peer *))
{
	for (auto &p : peers) {
		func(p.get());
	}
}

void Host::ForEachPeer(std::function<void(icon7::Peer *)> func)
{
	for (auto &p : peers) {
		func(p.get());
	}
}

void Host::WakeUp() {}

void Initialize() {}
void Deinitialize() {}

} // namespace icon7
