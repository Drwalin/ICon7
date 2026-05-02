// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_RPC_ENVIRONMENT_HPP
#define ICON7_RPC_ENVIRONMENT_HPP

#include <string>
#include <unordered_map>

#include "../../bitscpp/include/bitscpp/Endianness.hpp"

#include "ByteWriter.hpp"
#include "RpcName.hpp"
#include "Debug.hpp"
#include "ByteReader.hpp"
#include "Util.hpp"
#include "Flags.hpp"
#include "Peer.hpp"
#include "Host.hpp"
#include "Loop.hpp"
#include "CommandsBufferHandler.hpp"
#include "OnReturnCallback.hpp"
#include "MessageConverter.hpp"

namespace icon7
{
class CommandExecutionQueue;

namespace commands
{
namespace internal
{
class CommandCallSend final : public commands::ExecuteOnPeer
{
public:
	CommandCallSend(CommandCallSend &&o)
		: callback(std::move(o.callback)), buffer(std::move(o.buffer))
	{
		assert(_commandSize == sizeof(CommandCallSend));
		assert(buffer.data());
		peer = std::move(o.peer);
		flags = o.flags;
		o.flags = 0;
	}

	CommandCallSend(PeerHandle peer, OnReturnCallback &&callback,
					Flags flags, ByteBufferWritable &&buffer)
		: ExecuteOnPeer(peer), callback(std::move(callback)),
		  buffer(std::move(buffer)), flags(flags)
	{
		assert(this->buffer.data());
	}
	virtual ~CommandCallSend() {}

	OnReturnCallback callback;
	ByteBufferWritable buffer;
	Flags flags;

	virtual void Execute() override
	{
		assert(buffer.data());
		PeerData *peer = this->peer.GetLocalPeerData();
		const uint32_t rcbId = peer->_InternalGetNextValidReturnCallbackId();
		const uint32_t rcbId_v = bitscpp::HostToNetworkUint(rcbId);
		memcpy((void *)buffer.data(), &rcbId_v, sizeof(rcbId_v));
		peer->returningCallbacks.InsertOrSet(rcbId, std::move(callback));
		flags |= FLAGS_CALL;
		if (FramingProtocol::WriteHeaderIntoBuffer(buffer, flags)) {
			peer->Send(std::move(buffer));
		} else {
			LOG_FATAL("Trying to write header into invalid ByteBuffer.");
		}
	}
};
} // namespace internal
} // namespace commands

class RPCEnvironment
{
public:
	RPCEnvironment();
	~RPCEnvironment();

	void OnReceive(PeerData *peer, ByteBufferReadable &frameData,
				   uint32_t headerSize, Flags flags) const;
	/*
	 * expects that reader already filled flags fully and reader is at body
	 * offset
	 */
	void OnReceive(PeerData *peer, ByteReader &reader, Flags flags) const;
	void OnReceiveCall(PeerData *peer, ByteReader &reader, Flags flags) const;
	void OnReceiveReturn(PeerData *peer, ByteReader &reader, Flags flags) const;

	template <typename Fun>
	MessageConverter *
	RegisterMessage(RpcName name, std::function<Fun> fun,
					CommandExecutionQueue *executionQueue = nullptr,
					CommandExecutionQueue *(*getExecutionQueue)(
						MessageConverter *messageConverter, PeerHandle peer,
						ByteReader &reader, Flags flags) = nullptr)
	{
		auto func = new MessageConverterSpecStdFunction(fun);
		func->_executionQueue = executionQueue;
		func->getExecutionQueue = getExecutionQueue;
		return RegisterAnyMessage(name, func);
	}

	template <typename Fun>
	MessageConverter *
	RegisterMessage(RpcName name, Fun &&fun,
					CommandExecutionQueue *executionQueue = nullptr,
					CommandExecutionQueue *(*getExecutionQueue)(
						MessageConverter *messageConverter, PeerHandle peer,
						ByteReader &reader, Flags flags) = nullptr)
	{
		auto f = ConvertLambdaToFunctionPtr(fun);
		auto func = new MessageConverterSpec(f);
		func->_executionQueue = executionQueue;
		func->getExecutionQueue = getExecutionQueue;
		return RegisterAnyMessage(name, func);
	}

	template <typename T, typename Fun>
	MessageConverter *
	RegisterObjectMessage(RpcName name, T *object, Fun &&fun,
						  CommandExecutionQueue *executionQueue = nullptr,
						  CommandExecutionQueue *(*getExecutionQueue)(
							  MessageConverter *messageConverter, PeerHandle peer,
							  ByteReader &reader, Flags flags) = nullptr)
	{
		auto func = new MessageConverterSpecMethodOfObject(object, fun);
		func->_executionQueue = executionQueue;
		func->getExecutionQueue = getExecutionQueue;
		return RegisterAnyMessage(name, func);
	}

	MessageConverter *
	RegisterAnyMessage(RpcName name, MessageConverter *messageConverter);

	template <typename... Targs>
	static void Send(PeerHandle peer, Flags flags, const RpcName &name,
					 const Targs &...args)
	{
		ByteBufferWritable buffer(100);
		SerializeSend(buffer, flags, name, args...);
		Peer::Send(peer, std::move(buffer));
	}

	static void
	InitializeSerializeSend(bitscpp::v2::ByteWriter_ByteBuffer &writer,
							const RpcName &name)
	{
		assert(name);
		writer.op(name);
	}

	static void
	FinalizeSerializeSend(bitscpp::v2::ByteWriter_ByteBuffer &writer,
						  Flags &flags)
	{
		flags |= FLAGS_CALL_NO_FEEDBACK;
		if (FramingProtocol::WriteHeaderIntoBuffer(*writer._buffer, flags) ==
			false) {
			LOG_FATAL("Trying to write header into invalid ByteBuffer.");
		}
	}

	template <typename... Targs>
	static void SerializeSend(ByteBufferWritable &buffer, Flags flags,
							  const RpcName &name, const Targs &...args)
	{
		bitscpp::v2::ByteWriter_ByteBuffer writer(&buffer);
		InitializeSerializeSend(writer, name);
		(writer.op(args), ...);
		FinalizeSerializeSend(writer, flags);
	}

	template <typename... Targs>
	static ByteBufferReadable
	SerializeSend(Flags flags, const RpcName &name, const Targs &...args)
	{
		ByteBufferWritable buffer(100);
		SerializeSend(buffer, flags, name, args...);
		return buffer;
	}

	template <typename... Targs>
	static void Call(CommandsBufferHandler *commandsBuffer, PeerHandle peer,
					 Flags flags, OnReturnCallback &&callback,
					 const RpcName &name, const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op_untyped_uint32(0);
		InitializeSerializeSend(writer, name);
		(writer.op(args), ...);

		commandsBuffer->EnqueueCommand<commands::internal::CommandCallSend>(
			peer, std::move(callback), flags,
			std::move(writer._data));
	}

	template <typename... Targs>
	static void Call(PeerHandle peer, Flags flags, OnReturnCallback &&callback,
					 const RpcName &name, const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op_untyped_uint32(0);
		InitializeSerializeSend(writer, name);
		(writer.op(args), ...);

		auto cb = CommandHandle<commands::internal::CommandCallSend>::Create(
			peer, std::move(callback), flags,
			std::move(writer._data));
		peer.GetLoop()->EnqueueCommand(std::move(cb));
	}

	void RemoveRegisteredMessage(const RpcName &name);

	friend class Host;

	static void CheckForTimeoutFunctionCalls(Loop *loop, uint32_t maxChecks);

protected:
	std::unordered_map<std::string, MessageConverter *> registeredMessagesString;
	std::vector<MessageConverter *> registeredMessagesUint;
};
} // namespace icon7

#endif
