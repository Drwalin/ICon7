// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Loop.hpp"

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

void RPCEnvironment::OnReceive(Peer *peer, ByteBufferReadable &frameData,
							   uint32_t headerSize, Flags flags) const
{
	flags = FramingProtocol::GetPacketFlags(frameData.data(), flags);
	frameData.storage->flags = flags;
	ByteReader reader(std::move(frameData), headerSize);
	OnReceive(peer, reader, flags);
	std::swap(frameData, reader._data);
}

void RPCEnvironment::OnReceive(Peer *peer, ByteReader &reader,
							   Flags flags) const
{
	assert(reader.get_errors() == 0);
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

void RPCEnvironment::OnReceiveCall(Peer *peer, ByteReader &reader,
								   Flags flags) const
{
	uint32_t returnId = 0;
	if ((flags & 6) == FLAGS_CALL) {
		reader.op_untyped_uint32(returnId);
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
									 Flags flags) const
{
	uint32_t returnId = 0;
	reader.op_untyped_uint32(returnId);
	OnReturnCallback callback;
	auto cb = peer->returningCallbacks.Get(returnId);
	if (cb != nullptr) {
		callback = std::move(*cb);
		peer->returningCallbacks.Remove(returnId);
		callback.Execute(peer, flags, reader);
	} else {
		LOG_WARN("Remote function call returned value but OnReturnedCallback "
				 "already expired. returnId = %u",
				 returnId);
	}
}

void RPCEnvironment::CheckForTimeoutFunctionCalls(Loop *loop,
												  uint32_t maxChecks)
{
	auto now = time::GetTemporaryTimestamp();
	for (uint32_t i = 0; i < maxChecks; ++i) {
		Peer *peer = loop->_InternalGetRandomPeer();
		if (peer) {
			peer->CheckForTimeoutFunctionCalls(now);
		} else {
			return;
		}
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
} // namespace icon7
