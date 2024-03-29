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
namespace commands
{
void ExecuteOnPeer::Execute() { function(peer.get(), data, userPointer); }

void ExecuteOnPeerNoArgs::Execute() { function(peer.get()); }

void ExecuteAddPeerToFlush::Execute()
{
	host->_InternalInsertPeerToFlush(peer.get());
}

void ExecuteRPC::Execute()
{
	messageConverter->Call(peer.get(), reader, flags, returnId);
}

void ExecuteReturnRC::Execute()
{
	function(peer.get(), flags, reader, funcPtr);
}

void ExecuteBooleanOnHost::Execute() { function(host, result, userPointer); }

void ExecuteOnHost::Execute() { function(host, userPointer); }

void ExecuteConnect::Execute() { host->_InternalConnect(*this); }

void ExecuteListen::Execute()
{
	host->_InternalListen(ipProto, port, std::move(onListen));
}

void ExecuteDisconnect::Execute()
{
	if (peer->IsClosed() == false) {
		peer->_InternalDisconnect();
	} else {
	}
}

void ExecuteFunctionPointer::Execute() { function(); }
} // namespace commands

using namespace commands;

void Command::Execute()
{
	std::visit(
		[](auto &&c) -> int {
			if constexpr (std::is_same_v<std::decay_t<decltype(c)>, int> ==
						  false)
				c.Execute();
			return 0;
		},
		std::move(cmd));
	cmd = 0;
}
} // namespace icon7
