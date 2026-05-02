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
	for (auto it : registeredMessagesString) {
		delete it.second;
	}
	for (auto it : registeredMessagesUint) {
		delete it;
	}
	registeredMessagesString.clear();
	registeredMessagesUint.clear();
}

void RPCEnvironment::OnReceive(PeerData *peer, ByteBufferReadable &frameData,
							   uint32_t headerSize, Flags flags) const
{
	flags = FramingProtocol::GetPacketFlags(frameData.data(), flags);
	frameData.storage->flags = flags;
	ByteReader reader(std::move(frameData), headerSize);
	OnReceive(peer, reader, flags);
	std::swap(frameData, reader._data);
}

void RPCEnvironment::OnReceive(PeerData *peer, ByteReader &reader,
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

void RPCEnvironment::OnReceiveCall(PeerData *peer, ByteReader &reader,
								   Flags flags) const
{
	uint32_t returnId = 0;
	if ((flags & 6) == FLAGS_CALL) {
		reader.op_untyped_uint32(returnId);
	}
	RpcName name;
	reader.op(name);
	MessageConverter *msgConv = nullptr;

	if (name.IsString()) {
		auto it = registeredMessagesString.find(name.str);
		if (registeredMessagesString.end() != it) {
			msgConv = it->second;
		}
	} else if (name.IsUint()) {
		if (registeredMessagesUint.size() > name.uint) {
			msgConv = registeredMessagesUint[name.uint];
		}
	} else {
		LOG_WARN("Received RPC call without function name. Ignoring.");
	}

	if (msgConv) {
		auto queue = msgConv->ExecuteGetQueue(peer->peerHandle, reader, flags);
		if (queue) {
			auto com = CommandHandle<commands::internal::ExecuteRPC>::Create(
				std::move(reader), peer->peerHandle);
			com->flags = flags;
			com->returnId = returnId;
			com->messageConverter = msgConv;
			queue->EnqueueCommand(std::move(com));
		} else {
			msgConv->Call(peer->peerHandle, reader, flags, returnId);
		}
	} else {
		LOG_WARN("Function %s not registered.", name.ToString().c_str());
	}
}

void RPCEnvironment::OnReceiveReturn(PeerData *peer, ByteReader &reader,
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
		PeerHandle peer = loop->_InternalGetRandomPeer();
		if (peer) {
			peer.GetLocalPeerData()->CheckForTimeoutFunctionCalls(now);
		} else {
			return;
		}
	}
}

void RPCEnvironment::RemoveRegisteredMessage(const RpcName &name)
{
	MessageConverter *msgConv = nullptr;

	if (name.IsString()) {
		auto it = registeredMessagesString.find(name.str);
		if (registeredMessagesString.end() != it) {
			msgConv = it->second;
			registeredMessagesString.erase(it);
		}
	} else if (name.IsUint()) {
		if (registeredMessagesUint.size() > name.uint) {
			registeredMessagesUint[name.uint] = nullptr;
			msgConv = registeredMessagesUint[name.uint];
		}
		while (!registeredMessagesUint.empty()) {
			if (registeredMessagesUint.back() == nullptr) {
				registeredMessagesUint.pop_back();
			} else {
				break;
			}
		}
	}
	delete msgConv;
}

MessageConverter *
RPCEnvironment::RegisterAnyMessage(RpcName name,
								   MessageConverter *messageConverter)
{
	assert(name);
	messageConverter->name = name;
	if (name.IsString()) {
		if (registeredMessagesString.contains(name.str) == true) {
			LOG_FATAL("Message %s already registered!",
					  name.ToString().c_str());
		} else {
			registeredMessagesString[name.str] = messageConverter;
		}
	} else if (name.IsUint()) {
		if (registeredMessagesUint.size() <= name.uint) {
			registeredMessagesUint.resize(name.uint + 1, nullptr);
		}
		if (registeredMessagesUint[name.uint] != nullptr) {
			LOG_FATAL("Message %s already registered!",
					  name.ToString().c_str());
		}
		registeredMessagesUint[name.uint] = messageConverter;
	}
	return messageConverter;
}

} // namespace icon7
