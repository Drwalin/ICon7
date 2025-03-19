/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
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
	ByteReader reader(std::move(frameData), headerSize);
	OnReceive(peer, reader, flags);
	std::swap(frameData, reader._data);
}

void RPCEnvironment::OnReceive(Peer *peer, ByteReader &reader, Flags flags)
{
	switch (flags & 6) {
	case FLAGS_CALL:
	case FLAGS_CALL_NO_FEEDBACK:
		OnReceiveCall(peer, reader, flags);
		break;
	case FLAGS_CALL_RETURN_FEEDBACK:
		OnReceiveReturn(peer, reader, flags);
		break;
	default:
		LOG_WARN("Received packet with UNUSED RPC type bits set.");
	}
}

void RPCEnvironment::OnReceiveCall(Peer *peer, ByteReader &reader, Flags flags)
{
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
			auto com = CommandHandle<commands::internal::ExecuteRPC>::Create(
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
}

void RPCEnvironment::OnReceiveReturn(Peer *peer, ByteReader &reader,
									 Flags flags)
{
	uint32_t id;
	reader.op(id);
	OnReturnCallback callback;
	bool found = false;
	{
		auto it = returningCallbacks.find(id);
		if (it != returningCallbacks.end()) {
			auto it2 = it->second.find(peer);
			if (it2 != it->second.end()) {
				callback = std::move(it2->second);
				it->second.erase(it2);
				found = true;
			}
			if (it->second.empty()) {
				returningCallbacks.erase(it);
			}
		}
	}
	if (found) {
		callback.Execute(peer, flags, reader);
	} else {
		LOG_WARN("Remote function call returned value but OnReturnedCallback "
				 "already expired. returnId = %u",
				 id);
	}
}

void RPCEnvironment::CheckForTimeoutFunctionCalls(uint32_t maxChecks)
{
	auto now = std::chrono::steady_clock::now();
	auto it = returningCallbacks.find(lastCheckedId);
	if (it == returningCallbacks.end())
		it = returningCallbacks.begin();
	else
		++it;
	if (it == returningCallbacks.end())
		it = returningCallbacks.begin();
	for (int i = 0; i < maxChecks && it != returningCallbacks.end(); ++i) {
		lastCheckedId = it->first;
		for (auto it2 = it->second.begin(); it2 != it->second.end(); ++i) {
			if (it2->second.IsExpired(now)) {
				timeouts.push_back(std::move(it2->second));
				it2 = it->second.erase(it2);
			} else {
				++it2;
			}
		}
		if (it->second.empty()) {
			it = returningCallbacks.erase(it);
		} else {
			++it;
		}
	}
	for (auto &t : timeouts) {
		t.ExecuteTimeout();
	}
	timeouts.clear();
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

uint32_t RPCEnvironment::GetNewReturnIdCallback(Peer *peer)
{
	uint32_t rcbId = 0;
	for (;;) {
		rcbId = ++(peer->returnIdGen);
		if (rcbId != 0) {
			auto &r = returningCallbacks[rcbId];
			auto it = r.find(peer);
			if (it == r.end()) {
				break;
			}
		}
	};
	return rcbId;
}
} // namespace icon7
