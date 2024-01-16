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

#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/CommandExecutionQueue.hpp"
#include "../include/icon7/FramingProtocol.hpp"

#include "../include/icon7/RPCEnvironment.hpp"

namespace icon7
{
RPCEnvironment::~RPCEnvironment()
{
	for (auto it : registeredMessages) {
		delete it.second;
	}
	registeredMessages.clear();
}

void RPCEnvironment::OnReceive(Peer *peer, std::vector<uint8_t> &frameData,
							   uint32_t headerSize, Flags flags)
{
	flags = FramingProtocol::GetPacketFlags(frameData.data(), flags);
	ByteReader reader(std::move(frameData), headerSize);
	OnReceive(peer, reader, flags);
	std::swap(frameData, reader._data);
}

void RPCEnvironment::OnReceive(Peer *peer, ByteReader &reader, Flags flags)
{
	switch (flags & 6) {
	case FLAGS_CALL:
	case FLAGS_CALL_NO_FEEDBACK: {
		uint32_t returnId = 0;
		if ((flags & 6) == FLAGS_CALL) {
			reader.op(returnId);
		}
		std::string name;
		reader.op(name);
		auto it = registeredMessages.find(name);
		if (registeredMessages.end() != it) {
			auto mtd = it->second;
			if (mtd->executionQueue) {
				Command command{commands::ExecuteRPC(std::move(reader))};
				commands::ExecuteRPC &com =
					std::get<commands::ExecuteRPC>(command.cmd);
				com.peer = peer->shared_from_this();
				com.flags = flags;
				com.returnId = returnId;
				com.messageConverter = mtd;
				mtd->executionQueue->EnqueueCommand(std::move(command));
			} else {
				mtd->Call(peer, reader, flags, returnId);
			}
		} else {
		}
	} break;
	case FLAGS_CALL_RETURN_FEEDBACK: {
		uint32_t id;
		reader.op(id);
		OnReturnCallback callback;
		bool found = false;
		{
			std::lock_guard guard{mutexReturningCallbacks};
			auto it2 = returningCallbacks.find(id);
			if (it2 != returningCallbacks.end()) {
				callback = std::move(it2->second);
				returningCallbacks.erase(it2);
				found = true;
			}
		}
		if (found) {
			callback.Execute(peer, flags, reader);
		} else {
			DEBUG("Remote function call returned value but OnReturnedCallback "
				  "already expired.");
		}
	} break;
	default:
		DEBUG("Received packet with UNUSED RPC type bits set.");
	}
}

void RPCEnvironment::CheckForTimeoutFunctionCalls(uint32_t maxChecks)
{
	std::vector<OnReturnCallback> timeouts;
	auto now = std::chrono::steady_clock::now();
	if (mutexReturningCallbacks.try_lock()) {
		auto it = returningCallbacks.find(lastCheckedId);
		if (it == returningCallbacks.end())
			it = returningCallbacks.begin();
		for (int i = 0; i < maxChecks && it != returningCallbacks.end(); ++i) {
			lastCheckedId = it->first;
			if (it->second.IsExpired(now)) {
				auto next = it;
				++next;
				uint32_t nextId = 0;
				if (next != returningCallbacks.end()) {
					nextId = next->first;
				}
				timeouts.push_back(std::move(it->second));
				returningCallbacks.erase(lastCheckedId);
				it = returningCallbacks.find(nextId);
			} else {
				++it;
			}
		}
		mutexReturningCallbacks.unlock();
	}
	for (auto &t : timeouts) {
		t.ExecuteTimeout();
	}
}
} // namespace icon7
