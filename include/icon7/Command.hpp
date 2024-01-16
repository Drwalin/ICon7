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

#include <cinttypes>

#include <variant>
#include <memory>
#include <vector>

#include "Flags.hpp"
#include "ByteReader.hpp"
#include "ByteWriter.hpp"

namespace icon7
{

class Host;
class Peer;
class MessageConverter;

class Command;
class CommandExecutionQueue;

// TODO: remove copy constructors/operators

namespace commands
{
class ExecuteOnPeerNoArgs final
{
public:
	ExecuteOnPeerNoArgs(ExecuteOnPeerNoArgs &&) = default;
	ExecuteOnPeerNoArgs &operator=(ExecuteOnPeerNoArgs &&) = default;

	std::shared_ptr<Peer> peer;
	void (*function)(Peer *peer);

	void Execute();
};

class ExecuteOnPeer final
{
public:
	ExecuteOnPeer() = default;
	ExecuteOnPeer(ExecuteOnPeer &&) = default;
	ExecuteOnPeer &operator=(ExecuteOnPeer &&) = default;

	std::shared_ptr<Peer> peer;
	std::vector<uint8_t> data;
	void *userPointer;
	void (*function)(Peer *peer, std::vector<uint8_t> &data,
					 void *customSharedData);

	void Execute();
};

class ExecuteAddPeerToFlush final
{
public:
	ExecuteAddPeerToFlush() = default;
	ExecuteAddPeerToFlush(ExecuteAddPeerToFlush &&) = default;
	ExecuteAddPeerToFlush &operator=(ExecuteAddPeerToFlush &&) = default;

	std::shared_ptr<Peer> peer;
	Host *host;

	void Execute();
};

class ExecuteRPC final
{
public:
	ExecuteRPC &operator=(ExecuteRPC &&) = default;

	ExecuteRPC(ByteReader &&reader) : reader(std::move(reader)) {}
	ExecuteRPC(ExecuteRPC &&o)
		: peer(std::move(o.peer)), reader(std::move(o.reader)),
		  messageConverter(std::move(o.messageConverter)), flags(o.flags)
	{
	}

	std::shared_ptr<Peer> peer;
	ByteReader reader;
	MessageConverter *messageConverter;
	Flags flags;
	uint32_t returnId;

	void Execute();
};

class ExecuteReturnRC final
{
public:
	ExecuteReturnRC &operator=(ExecuteReturnRC &&) = default;

	ExecuteReturnRC(ByteReader &&reader) : reader(std::move(reader)) {}
	ExecuteReturnRC(ExecuteReturnRC &&o)
		: peer(std::move(o.peer)), reader(std::move(o.reader)),
		  function(std::move(o.function)), flags(o.flags)
	{
	}

	std::shared_ptr<Peer> peer;
	void *funcPtr;
	ByteReader reader;
	void (*function)(Peer *, Flags, ByteReader &, void *);
	Flags flags;

	void Execute();
};

class ExecuteBooleanOnHost final
{
public:
	ExecuteBooleanOnHost() = default;
	ExecuteBooleanOnHost(ExecuteBooleanOnHost &&) = default;
	ExecuteBooleanOnHost &operator=(ExecuteBooleanOnHost &&) = default;

	Host *host;
	void *userPointer;
	bool result;

	void (*function)(Host *, bool, void *);

	void Execute();
};

class ExecuteOnHost final
{
public:
	ExecuteOnHost() = default;
	ExecuteOnHost(ExecuteOnHost &&) = default;
	ExecuteOnHost &operator=(ExecuteOnHost &&) = default;

	Host *host;
	void *userPointer;

	void (*function)(Host *, void *);

	void Execute();
};

class ExecuteConnect final
{
public:
	ExecuteConnect(ExecuteConnect &&) = default;
	ExecuteConnect &operator=(ExecuteConnect &&) = default;

	Host *host;
	std::string address;
	uint16_t port;

	CommandExecutionQueue *executionQueue;
	ExecuteOnPeer onConnected;

	void Execute();
};

class ExecuteDisconnect final
{
public:
	ExecuteDisconnect(ExecuteDisconnect &&) = default;
	ExecuteDisconnect &operator=(ExecuteDisconnect &&) = default;

	std::shared_ptr<Peer> peer;

	void Execute();
};

class ExecuteFunctionPointer final
{
public:
	ExecuteFunctionPointer(ExecuteFunctionPointer &&) = default;
	ExecuteFunctionPointer &operator=(ExecuteFunctionPointer &&) = default;

	void (*function)();

	void Execute();
};
} // namespace commands

class Command final
{
public:
	Command() : cmd(0) {}
	~Command() = default;
	Command(Command &&other) = default;
	Command(Command &other) = delete;
	Command(const Command &other) = delete;

	Command &operator=(Command &&other) = default;
	Command &operator=(Command &other) = delete;
	Command &operator=(const Command &other) = delete;

	std::variant<int, commands::ExecuteOnPeer, commands::ExecuteOnPeerNoArgs,
				 commands::ExecuteAddPeerToFlush, commands::ExecuteRPC,
				 commands::ExecuteReturnRC, commands::ExecuteBooleanOnHost,
				 commands::ExecuteConnect, commands::ExecuteDisconnect,
				 commands::ExecuteFunctionPointer, commands::ExecuteOnHost>
		cmd;

	void Execute();

public:
	Command(commands::ExecuteOnHost &&executeOnHost)
		: cmd(std::move(executeOnHost))
	{
	}
	Command(commands::ExecuteOnPeer &&executeOnPeer)
		: cmd(std::move(executeOnPeer))
	{
	}
	Command(commands::ExecuteAddPeerToFlush &&executeAddPeerToFlush)
		: cmd(std::move(executeAddPeerToFlush))
	{
	}
	Command(commands::ExecuteOnPeerNoArgs &&executeOnPeerNoArgs)
		: cmd(std::move(executeOnPeerNoArgs))
	{
	}
	Command(commands::ExecuteRPC &&executeRPC) : cmd(std::move(executeRPC)) {}
	Command(commands::ExecuteReturnRC &&executeReturnRC)
		: cmd(std::move(executeReturnRC))
	{
	}
	Command(commands::ExecuteBooleanOnHost &&executeBooleanOnHost)
		: cmd(std::move(executeBooleanOnHost))
	{
	}
	Command(commands::ExecuteConnect &&executeConnect)
		: cmd(std::move(executeConnect))
	{
	}
	Command(commands::ExecuteDisconnect &&executeDisconnect)
		: cmd(std::move(executeDisconnect))
	{
	}
	Command(commands::ExecuteFunctionPointer &&executeFunctionPointer)
		: cmd(std::move(executeFunctionPointer))
	{
	}
};
} // namespace icon7

#endif
