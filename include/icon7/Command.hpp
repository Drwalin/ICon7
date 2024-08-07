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

#ifndef ICON7_COMMAND_HPP
#define ICON7_COMMAND_HPP

#include <memory>
#include <type_traits>

#include "Flags.hpp"
#include "MemoryPool.hpp"
#include "ByteReader.hpp"

namespace icon7
{
class Host;
class Peer;
class MessageConverter;

class CommandExecutionQueue;

class Command
{
public:
	inline Command() {}

	Command(Command &&other) = delete;
	Command(Command &other) = delete;
	Command(const Command &other) = delete;

	Command &operator=(Command &&other) = delete;
	Command &operator=(Command &other) = delete;
	Command &operator=(const Command &other) = delete;

	virtual void Execute() = 0;

	uint32_t _commandSize = 0;

	virtual ~Command();
};

template <typename T = Command> class CommandHandle
{
public:
	CommandHandle() : _com(nullptr) {}
	template <typename T2> CommandHandle(CommandHandle<T2> &&o)
	{
		static_assert(std::is_base_of_v<T, T2>);
		_com = o._com;
		o._com = nullptr;
	}
	CommandHandle(CommandHandle &o) = delete;
	CommandHandle(const CommandHandle &o) = delete;

	inline ~CommandHandle() {}

	CommandHandle &operator=(CommandHandle &o) = delete;
	CommandHandle &operator=(const CommandHandle &o) = delete;
	template <typename T2> CommandHandle &operator=(CommandHandle<T2> &&o)
	{
		static_assert(std::is_base_of_v<T, T2>);
		this->~CommandHandle();
		_com = o._com;
		o._com = nullptr;
		return *this;
	}

	void Destroy()
	{
		if (_com != nullptr) {
			MemoryPool::ReleaseTyped(_com, _com->_commandSize);
			_com = nullptr;
		}
	}

	template <typename... Args>
	inline static CommandHandle<T> Create(Args &&...args)
	{
		CommandHandle<T> com;
		com._com = MemoryPool::AllocateTyped<T>(std::move(args)...);
		com._com->_commandSize = sizeof(T);
		return com;
	}

	inline void Execute()
	{
		if (_com != nullptr) {
			_com->Execute();
			Destroy();
		}
	}

	inline bool IsValid() const { return _com != nullptr; }

	inline T &operator*(int) { return *_com; }
	inline T *operator->() { return _com; }

	friend class CommandExecutionQueue;

public:
	T *_com = nullptr;
};

namespace commands
{
class ExecuteOnPeer : public Command
{
public:
	virtual ~ExecuteOnPeer() {}
	ExecuteOnPeer() {}
	ExecuteOnPeer(std::shared_ptr<Peer> &peer) : peer(peer) {}

	std::shared_ptr<Peer> peer;
};

class ExecuteOnHost : public Command
{
public:
	virtual ~ExecuteOnHost() {}
	ExecuteOnHost() {}
	ExecuteOnHost(Host *host) : host(host) {}

	Host *host = nullptr;
};

class ExecuteBooleanOnHost : public ExecuteOnHost
{
public:
	virtual ~ExecuteBooleanOnHost() {}
	ExecuteBooleanOnHost() {}
	ExecuteBooleanOnHost(Host *host, bool result)
		: ExecuteOnHost(host), result(result)
	{
	}

	bool result = false;
};

namespace internal
{
class ExecuteRPC : public Command
{
public:
	virtual ~ExecuteRPC() {}
	ExecuteRPC(ByteReader &&reader) : reader(std::move(reader)) {}

	std::shared_ptr<Peer> peer;
	ByteReader reader;
	MessageConverter *messageConverter = nullptr;
	Flags flags = 0;
	uint32_t returnId = 0;

	virtual void Execute() override;
};

class ExecuteAddPeerToFlush : public Command
{
public:
	virtual ~ExecuteAddPeerToFlush() {}
	ExecuteAddPeerToFlush() {}
	ExecuteAddPeerToFlush(std::shared_ptr<Peer> peer, Host *host)
		: peer(peer), host(host)
	{
	}

	std::shared_ptr<Peer> peer;
	Host *host = nullptr;

	virtual void Execute() override;
};

class ExecuteListen : public Command
{
public:
	virtual ~ExecuteListen() {}

	ExecuteListen() {}

	std::string address;
	IPProto ipProto;
	Host *host = nullptr;
	uint16_t port = 0;

	CommandHandle<ExecuteBooleanOnHost> onListen;
	CommandExecutionQueue *queue = nullptr;

	virtual void Execute() override;
};

class ExecuteConnect : public Command
{
public:
	virtual ~ExecuteConnect() {}
	ExecuteConnect() {}

	Host *host = nullptr;
	std::string address;
	uint16_t port = 0;

	CommandExecutionQueue *executionQueue;
	CommandHandle<ExecuteOnPeer> onConnected;

	virtual void Execute() override;
};

class ExecuteDisconnect : public Command
{
public:
	virtual ~ExecuteDisconnect() {}
	ExecuteDisconnect() {}

	std::shared_ptr<Peer> peer;

	virtual void Execute() override;
};
} // namespace internal
} // namespace commands
} // namespace icon7

#endif
