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
#include <vector>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/RPCEnvironment.hpp"

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
}

Host::~Host()
{
	WaitStopRunning();
	asyncRunner.join();
}

void Host::WaitStopRunning()
{
	QueueStopRunning();
	while (IsRunningAsync()) {
		QueueStopRunning();
		WakeUp();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Host::_InternalDestroy()
{
	DisconnectAllAsync();
	StopListening();
	QueueStopRunning();
	while (IsRunningAsync()) {
		DisconnectAllAsync();
		StopListening();
		QueueStopRunning();
		WakeUp();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	DispatchAllEventsFromQueue(128);
	while (commandQueue.HasAny()) {
		commandQueue.Execute(128);
	}
}

void Host::DisconnectAllAsync()
{
	this->EnqueueCommand(CommandHandle<commands::ExecuteOnHost>::Create(
		this, nullptr, [](Host *host, void *) { host->DisconnectAll(); }));
}

void Host::DisconnectAll()
{
	for (auto &p : peers) {
		p->Disconnect();
	}
}

uint32_t Host::DispatchAllEventsFromQueue(uint32_t maxEvents)
{
	return commandQueue.Execute(maxEvents);
}

std::future<std::shared_ptr<Peer>> Host::ConnectPromise(std::string address,
														uint16_t port)
{
	std::promise<std::shared_ptr<Peer>> *promise =
		new std::promise<std::shared_ptr<Peer>>();
	auto onConnected = CommandHandle<commands::ExecuteOnPeer>::Create();
	onConnected->userPointer = promise;
	onConnected->function = [](auto peer, auto data, auto userPointer) {
		std::promise<std::shared_ptr<Peer>> *promise =
			(std::promise<std::shared_ptr<Peer>> *)(userPointer);
		if (peer != nullptr) {
			promise->set_value(peer->shared_from_this());
		} else {
			promise->set_value(nullptr);
		}
		delete promise;
	};
	std::future<std::shared_ptr<Peer>> future = promise->get_future();
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
	peersToFlush.erase(peer);
	peer->peerFlags |= Peer::BIT_CLOSED;
}

std::future<bool> Host::ListenOnPort(const std::string &address, uint16_t port,
									 IPProto ipProto)
{
	std::promise<bool> *promise = new std::promise<bool>();
	auto callback = CommandHandle<commands::ExecuteBooleanOnHost>::Create();
	callback->host = this;
	callback->userPointer = promise;
	callback->function = [](Host *host, bool result, void *userPointer) {
		std::promise<bool> *promise = (std::promise<bool> *)(userPointer);
		promise->set_value(result);
		delete promise;
	};
	auto future = promise->get_future();
	ListenOnPort(address, port, ipProto, std::move(callback), nullptr);
	return future;
}

void Host::ListenOnPort(
	const std::string &address, uint16_t port, IPProto ipProto,
	CommandHandle<commands::ExecuteBooleanOnHost> &&callback,
	CommandExecutionQueue *queue)
{
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
	auto com = CommandHandle<commands::ExecuteOnPeer>::Create();
	com->function = [](auto a, auto b, auto c) {};
	Connect(address, port, std::move(com), nullptr);
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
	if (peersToFlush.empty() && commandQueue.HasAny() == false) {
		if (peers.size() < 20) {
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	DispatchAllEventsFromQueue(128);
	std::vector<std::shared_ptr<Peer>> toRemoveFromQueue;
	toRemoveFromQueue.reserve(64);
	for (auto p : peersToFlush) {
		p->_InternalOnWritable();
		toRemoveFromQueue.emplace_back(p);
	}
	for (auto &p : toRemoveFromQueue) {
		if (p->_InternalHasQueuedSends() == false) {
			peersToFlush.erase(p);
		}
	}
	rpcEnvironment->CheckForTimeoutFunctionCalls(16);
}

void Host::InsertPeerToFlush(Peer *peer)
{
	auto com = CommandHandle<commands::internal::ExecuteAddPeerToFlush>::Create(
		peer->shared_from_this(), this);
	EnqueueCommand(std::move(com));
}

void Host::_InternalInsertPeerToFlush(Peer *peer)
{
	peersToFlush.insert(peer->shared_from_this());
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
