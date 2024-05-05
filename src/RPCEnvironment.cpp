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
RPCEnvironment::RPCEnvironment() {}

RPCEnvironment::~RPCEnvironment()
{
	for (auto it : registeredMessages) {
		delete it.second;
	}
	registeredMessages.clear();
}

void RPCEnvironment::OnReceive(Peer *peer, ByteBuffer &frameData,
							   uint32_t headerSize, Flags flags)
{
	flags = FramingProtocol::GetPacketFlags(frameData.data(), flags);
	ByteReader reader(frameData, headerSize);
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
			auto queue = mtd->ExecuteGetQueue(peer, reader, flags);
			if (queue) {
				auto com =
					CommandHandle<commands::internal::ExecuteRPC>::Create(
						std::move(reader));
				com->peer = peer->shared_from_this();
				com->flags = flags;
				com->returnId = returnId;
				com->messageConverter = mtd;
				queue->EnqueueCommand(std::move(com));
			} else {
				mtd->Call(peer, reader, flags, returnId);
			}
		} else {
			LOG_WARN("function not found: `%s`", name.c_str());
		}
	} break;
	case FLAGS_CALL_RETURN_FEEDBACK: {
		uint32_t id;
		reader.op(id);
		OnReturnCallback callback;
		bool found = false;
		{
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
			LOG_WARN(
				"Remote function call returned value but OnReturnedCallback "
				"already expired. returnId = %u/%u",
				id, this->returnCallCallbackIdGenerator);
		}
	} break;
	default:
		LOG_WARN("Received packet with UNUSED RPC type bits set.");
	}
}

void RPCEnvironment::CheckForTimeoutFunctionCalls(uint32_t maxChecks)
{
	timeouts.clear();
	auto now = std::chrono::steady_clock::now();
	auto it = returningCallbacks.find(lastCheckedId);
	if (it == returningCallbacks.end())
		it = returningCallbacks.begin();
	for (int i = 0; i < maxChecks && it != returningCallbacks.end(); ++i) {
		lastCheckedId = it->first;
		if (it->second.IsExpired(now)) {
			std::unordered_map<uint32_t, OnReturnCallback>::iterator next = it;
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
	for (auto &t : timeouts) {
		t.ExecuteTimeout();
	}
}

void RPCEnvironment::RemoveRegisteredMessage(const std::string &name)
{
	auto it = registeredMessages.find(name);
	if (it == registeredMessages.end()) {
		return;
	}
	delete it->second;
	registeredMessages.erase(it);
}

uint32_t RPCEnvironment::GetNewReturnIdCallback()
{
	uint32_t rcbId = 0;
	do {
		rcbId = ++returnCallCallbackIdGenerator;
	} while (rcbId == 0 || returningCallbacks.count(rcbId) != 0);
	return rcbId;
}
} // namespace icon7
