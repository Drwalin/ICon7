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

#ifndef ICON7_HOST_HPP
#define ICON7_HOST_HPP

#include <chrono>
#include <string>
#include <unordered_set>
#include <functional>

#include "../../concurrent/future.hpp"

#include "Flags.hpp"
#include "CommandExecutionQueue.hpp"
#include "Forward.hpp"

namespace icon7
{
class Peer;
class RPCEnvironment;

class Host
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

	void SetOnConnect(void (*callback)(Peer *));
	void SetOnDisconnect(void (*callback)(Peer *));

	concurrent::future<std::shared_ptr<Peer>>
	ConnectPromise(std::string address, uint16_t port);
	void Connect(std::string address, uint16_t port);
	void Connect(std::string address, uint16_t port,
				 CommandHandle<commands::ExecuteOnPeer> &&onConnected,
				 CommandExecutionQueue *queue = nullptr);

	virtual void StopListening() = 0;

	void RunAsync();
	void WaitStopRunning();
	void QueueStopRunning();
	bool IsRunningAsync();
	bool IsQueuedStopAsync();

	CommandExecutionQueue *GetCommandExecutionQueue() { return &commandQueue; }
	void EnqueueCommand(CommandHandle<Command> &&command);

	void InsertPeerToFlush(Peer *peer);

	RPCEnvironment *GetRpcEnvironment();
	void SetRpcEnvironment(RPCEnvironment *env);

	virtual void WakeUp();

public: // thread unsafe, safe only in hosts loop thread
	void ForEachPeer(void(func)(icon7::Peer *));
	void ForEachPeer(std::function<void(icon7::Peer *)> func);
	void DisconnectAll();

	virtual void SingleLoopIteration() = 0;
	virtual void _InternalSingleLoopIteration(bool forceExecution);

	void _InternalInsertPeerToFlush(Peer *peer);
	virtual void
	_InternalConnect(commands::internal::ExecuteConnect &connectCommand) = 0;
	virtual void
	_InternalListen(const std::string &address, IPProto ipProto, uint16_t port,
					CommandHandle<commands::ExecuteBooleanOnHost> &com) = 0;
	void
	_InternalConnect_Finish(commands::internal::ExecuteConnect &connectCommand);
	virtual void _Internal_on_open_Finish(std::shared_ptr<Peer> peer);
	void _Internal_on_close_Finish(std::shared_ptr<Peer> peer);

public:
	uint64_t userData;
	void *userPointer;

protected:
	Host();

	friend class Peer;

protected:
	void (*onConnect)(Peer *);
	void (*onDisconnect)(Peer *);

	RPCEnvironment *rpcEnvironment;

	std::unordered_set<std::shared_ptr<Peer>> peers;

	CommandExecutionQueue commandQueue;
	std::atomic<uint32_t> asyncRunnerFlags;
	std::thread asyncRunner;

protected:
	std::vector<std::shared_ptr<Peer>> toRemoveFromQueue;
	std::chrono::steady_clock::time_point timePointToExecuteLoop;

public:
	decltype(std::chrono::microseconds()) minimumSingleLoopExecutionPeriod;
};

void Initialize();
void Deinitialize();
} // namespace icon7

#endif
