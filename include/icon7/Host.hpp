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

#include <string>
#include <future>
#include <unordered_set>

#include "Flags.hpp"
#include "ByteReader.hpp"
#include "Command.hpp"
#include "CommandExecutionQueue.hpp"

#define DEBUG(...)                                                             \
	icon7::Debug(__FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

namespace icon7
{
void Debug(const char *file, int line, const char *function, const char *fmt,
		   ...);

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

	virtual std::future<bool> ListenOnPort(uint16_t port, IPProto ipProto);
	void ListenOnPort(uint16_t port, IPProto ipProto,
					  commands::ExecuteBooleanOnHost &&callback,
					  CommandExecutionQueue *queue = nullptr);

	void SetOnConnect(void (*callback)(Peer *)) { onConnect = callback; }
	void SetOnDisconnect(void (*callback)(Peer *)) { onDisconnect = callback; }

	std::future<std::shared_ptr<Peer>> ConnectPromise(std::string address,
													  uint16_t port);
	void Connect(std::string address, uint16_t port);
	void Connect(std::string address, uint16_t port,
				 commands::ExecuteOnPeer &&onConnected,
				 CommandExecutionQueue *queue = nullptr);

	virtual void StopListening() = 0;

	void RunAsync();
	void WaitStopRunning();
	void QueueStopRunning();
	bool IsRunningAsync();
	bool IsQueuedStopAsync();

	CommandExecutionQueue *GetCommandExecutionQueue() { return &commandQueue; }
	void EnqueueCommand(Command &&command);

	void InsertPeerToFlush(Peer *peer);

	inline RPCEnvironment *GetRpcEnvironment() { return rpcEnvironment; }
	inline void SetRpcEnvironment(RPCEnvironment *env) { rpcEnvironment = env; }

	virtual void WakeUp();

public: // thread unsafe, safe only in hosts loop thread
	void ForEachPeer(void(func)(icon7::Peer *));
	void ForEachPeer(std::function<void(icon7::Peer *)> func);
	uint32_t DispatchAllEventsFromQueue(uint32_t maxEventsDispatched = 100);
	void DisconnectAll();

	virtual void SingleLoopIteration() = 0;
	virtual void _InternalSingleLoopIteration();

	void _InternalInsertPeerToFlush(Peer *peer);
	virtual void _InternalConnect(commands::ExecuteConnect &connectCommand) = 0;
	virtual void _InternalListen(IPProto ipProto, uint16_t port,
								 commands::ExecuteBooleanOnHost &com) = 0;
	void _InternalConnect_Finish(commands::ExecuteConnect &connectCommand);
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
	std::unordered_set<std::shared_ptr<Peer>> peersToFlush;

	CommandExecutionQueue commandQueue;
	std::atomic<uint32_t> asyncRunnerFlags;
	std::thread asyncRunner;
};

void Initialize();
void Deinitialize();
} // namespace icon7

#endif
