// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_HOST_HPP
#define ICON7_HOST_HPP

#include <string>
#include <functional>

#include "../../concurrent/future.hpp"

#include "Stats.hpp"
#include "Flags.hpp"
#include "CommandExecutionQueue.hpp"
#include "PeerManager.hpp"
#include "Forward.hpp"

namespace icon7
{
class RPCEnvironment;
class Loop;

class Host : public std::enable_shared_from_this<Host>
{
public:
	virtual ~Host();

	Host(Host &) = delete;
	Host(Host &&) = delete;
	Host(const Host &) = delete;
	Host &operator=(Host &) = delete;
	Host &operator=(Host &&) = delete;
	Host &operator=(const Host &) = delete;

	virtual void _InternalDestroy();
	void DisconnectAllAsync();

	virtual concurrent::future<bool>
	ListenOnPort(const std::string &address, uint16_t port, IPProto ipProto);
	void ListenOnPort(const std::string &address, uint16_t port,
					  IPProto ipProto,
					  CommandHandle<commands::ExecuteBooleanOnHost> &&callback,
					  CommandExecutionQueue *queue = nullptr);

	void SetOnConnect(void (*callback)(PeerHandle));
	void SetOnDisconnect(void (*callback)(PeerHandle));

	concurrent::future<PeerHandle>
	ConnectPromise(std::string address, uint16_t port);
	void Connect(std::string address, uint16_t port);
	void Connect(std::string address, uint16_t port,
				 CommandHandle<commands::ExecuteOnPeer> &&onConnected,
				 CommandExecutionQueue *queue = nullptr);

	virtual void StopListening() = 0;

	CommandExecutionQueue *GetCommandExecutionQueue() { return commandQueue; }
	void EnqueueCommand(CommandHandle<Command> &&command);

	const RPCEnvironment *GetRpcEnvironment();

	Loop *GetLoop();

public: // thread unsafe, safe only in hosts loop thread
	void ForEachPeer(void(func)(PeerHandle));
	void ForEachPeer(std::function<void(PeerHandle)> func);
	void DisconnectAll();

	virtual void _InternalSingleLoopIteration();

	virtual void
	_InternalConnect(commands::internal::ExecuteConnect &connectCommand) = 0;
	virtual void
	_InternalListen(const std::string &address, IPProto ipProto, uint16_t port,
					CommandHandle<commands::ExecuteBooleanOnHost> &com) = 0;
	void
	_InternalConnect_Finish(commands::internal::ExecuteConnect &connectCommand);
	virtual void _Internal_on_open_Finish(PeerHandle peer);
	void _Internal_on_close_Finish(PeerHandle peer);

	virtual void _InternalStopListening() = 0;

	PeerHandle _InternalGetRandomPeer();

public:
	uint64_t userData;
	void *userPointer;
	std::shared_ptr<void> userSmartPtr;

protected:
	Host(std::string objectName, RPCEnvironment *rpcEnvironment, std::shared_ptr<Loop> loop);

	friend class Peer;
	friend class PeerData;

private:
// 	void FlushPendingPeers();

protected:
	void (*onConnect)(PeerHandle);
	void (*onDisconnect)(PeerHandle);

	const RPCEnvironment *rpcEnvironment;

	PeerReferences peerReferences;

	CommandExecutionQueue *commandQueue;

public:
	std::shared_ptr<Loop> loop;

public:
	HostStats stats;
	const std::string objectName;
};

void Initialize();
void Deinitialize();
} // namespace icon7

#endif
