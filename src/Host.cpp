// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Loop.hpp"

#include "../include/icon7/Host.hpp"

namespace icon7
{
Host::Host()
{
	userData = 0;
	userPointer = nullptr;
	onConnect = nullptr;
	onDisconnect = nullptr;
	rpcEnvironment = nullptr;
}

Host::~Host()
{
	if (rpcEnvironment) {
		rpcEnvironment->host = this;
	}
}

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
	std::vector<std::shared_ptr<Peer>> copy(peers.begin(), peers.end());
	for (auto &p : copy) {
		p->_InternalDisconnect();
	}
}

void Host::SetOnConnect(void (*callback)(Peer *)) { onConnect = callback; }
void Host::SetOnDisconnect(void (*callback)(Peer *))
{
	onDisconnect = callback;
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
	loop->EnqueueCommand(std::move(command));
}

void Host::_InternalSingleLoopIteration()
{
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

void Initialize() {}
void Deinitialize() {}
} // namespace icon7
