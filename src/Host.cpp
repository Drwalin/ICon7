// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cassert>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Loop.hpp"
#include "../include/icon7/PeerManager.hpp"

#include "../include/icon7/Host.hpp"

namespace icon7
{
Host::Host(std::string objectName, RPCEnvironment *rpcEnvironment, std::shared_ptr<Loop> loop)
	: rpcEnvironment(rpcEnvironment), peerReferences(this, loop.get()), objectName(objectName)
{
	userData = 0;
	userPointer = nullptr;
	onConnect = nullptr;
	onDisconnect = nullptr;
	rpcEnvironment = nullptr;
}

Host::~Host() {}

void Host::_InternalDestroy()
{
	_InternalStopListening();
	DisconnectAll();
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
	std::vector<std::shared_ptr<PeerData>> copy(peerReferences.peers.begin(), peerReferences.peers.end());
	for (auto &p : copy) {
		p->_InternalDisconnect();
	}
}

void Host::SetOnConnect(void (*callback)(PeerHandle)) { onConnect = callback; }
void Host::SetOnDisconnect(void (*callback)(PeerHandle))
{
	onDisconnect = callback;
}

concurrent::future<PeerHandle>
Host::ConnectPromise(std::string address, uint16_t port)
{
	class CommandConnectPromise final : public commands::ExecuteOnPeer
	{
	public:
		CommandConnectPromise() {}
		virtual ~CommandConnectPromise() {}

		concurrent::promise<PeerHandle> promise;

		virtual void Execute() override
		{
			if (peer) {
				promise.set_value(peer);
			} else {
				promise.set_value({});
			}
		}
	};

	auto onConnected = CommandHandle<CommandConnectPromise>::Create();
	concurrent::future<PeerHandle> future =
		onConnected->promise.get_future();
	Connect(address, port, std::move(onConnected), nullptr);
	return future;
}

void Host::_InternalConnect_Finish(commands::internal::ExecuteConnect &com)
{
// 	if (com.onConnected->peer) {
// 		peerReferences.peers.insert(com.onConnected->peer.ptr);
// 	}

	if (com.executionQueue) {
		com.executionQueue->EnqueueCommand(std::move(com.onConnected));
	} else {
		com.onConnected.Execute();
	}
}

void Host::_Internal_on_open_Finish(PeerHandle peer)
{
// 	peers.insert(peer.ptr);

	if (onConnect) {
		onConnect(peer);
	}
	peer.GetLocalPeerData()->SetReadyToUse();
}

void Host::_Internal_on_close_Finish(PeerHandle peer)
{
	auto p = peer.GetLocalPeerData();
	p->sharedPeer->peerFlags |= PeerStatusBits::BIT_DISCONNECTING;
	p->_InternalOnDisconnect();
	auto peerDataHolder = this->peerReferences.Release(peer);
	p->_InternalClearInternalDataOnClose();
// 	assert(peerDataHolder.use_count() == 1);
// 	peersRefe.erase(p->shared_from_this());
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
	loop->EnqueueCommand(std::move(command));
}

void Host::_InternalSingleLoopIteration()
{
	assert(rpcEnvironment);
// 	FlushPendingPeers();
// 	RPCEnvironment::CheckForTimeoutFunctionCalls(loop.get(), 16);
}

// void Host::FlushPendingPeers()
// {
// 	for (auto &p : peers) {
// 		if (p->_InternalHasBufferedSends() || p->_InternalHasQueuedSends()) {
// 			p->_InternalOnWritable();
// 		}
// 	}
// }

const RPCEnvironment *Host::GetRpcEnvironment() { return rpcEnvironment; }

void Host::ForEachPeer(void(func)(icon7::PeerHandle))
{
	for (auto &p : peerReferences.peers) {
		func(p->peerHandle);
	}
}

void Host::ForEachPeer(std::function<void(icon7::PeerHandle)> func)
{
	for (auto &p : peerReferences.peers) {
		func(p->peerHandle);
	}
}

PeerHandle Host::_InternalGetRandomPeer()
{
	if (peerReferences.peers.size() == 0) {
		return {};
	}
	int r = rand() % peerReferences.peers.size();
	return peerReferences.peers[r]->peerHandle;
}

void Initialize() {}
void Deinitialize() {}
} // namespace icon7
