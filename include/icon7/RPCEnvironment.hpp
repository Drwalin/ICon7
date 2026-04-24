// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_RPC_ENVIRONMENT_HPP
#define ICON7_RPC_ENVIRONMENT_HPP

#include <string>
#include <unordered_map>

#include "ByteWriter.hpp"
#include "Debug.hpp"
#include "ByteReader.hpp"
#include "Util.hpp"
#include "Flags.hpp"
#include "Peer.hpp"
#include "Host.hpp"
#include "CommandsBufferHandler.hpp"
#include "OnReturnCallback.hpp"
#include "MessageConverter.hpp"
#include "../../bitscpp/include/bitscpp/Endianness.hpp"

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
		flags = o.flags;
		peer = std::move(o.peer);
	}

	CommandCallSend(std::shared_ptr<Peer> &&peer, OnReturnCallback &&callback,
					Flags flags, ByteBuffer &&buffer)
		: ExecuteOnPeer(peer), callback(std::move(callback)), flags(flags),
		  buffer(std::move(buffer))
	{
	}
	virtual ~CommandCallSend() {}

	OnReturnCallback callback;
	Flags flags;
	ByteBuffer buffer;

	virtual void Execute() override
	{
		const uint32_t rcbId = peer->_InternalGetNextValidReturnCallbackId();
		const uint32_t rcbId_v = bitscpp::HostToNetworkUint(rcbId);
		memcpy(buffer.data(), &rcbId_v, sizeof(rcbId_v));
		peer->returningCallbacks.InsertOrSet(rcbId, std::move(callback));
		flags |= FLAGS_CALL;
		if (FramingProtocol::WriteHeaderIntoBuffer(buffer, flags)) {
			peer->SendLocalThread(std::move(buffer));
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

	void OnReceive(Peer *peer, ByteBuffer &frameData, uint32_t headerSize,
				   Flags flags) const;
	/*
	 * expects that reader already filled flags fully and reader is at body
	 * offset
	 */
	void OnReceive(Peer *peer, ByteReader &reader, Flags flags) const;
	void OnReceiveCall(Peer *peer, ByteReader &reader, Flags flags) const;
	void OnReceiveReturn(Peer *peer, ByteReader &reader, Flags flags) const;

	template <typename Fun>
	MessageConverter *
	RegisterMessage(const std::string &name, std::function<Fun> fun,
					CommandExecutionQueue *executionQueue = nullptr,
					CommandExecutionQueue *(*getExecutionQueue)(
						MessageConverter *messageConverter, Peer *peer,
						ByteReader &reader, Flags flags) = nullptr)
	{
		auto func = new MessageConverterSpecStdFunction(fun);
		func->_executionQueue = executionQueue;
		func->getExecutionQueue = getExecutionQueue;
		return RegisterAnyMessage(name, func);
	}

	template <typename Fun>
	MessageConverter *
	RegisterMessage(const std::string &name, Fun &&fun,
					CommandExecutionQueue *executionQueue = nullptr,
					CommandExecutionQueue *(*getExecutionQueue)(
						MessageConverter *messageConverter, Peer *peer,
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
	RegisterObjectMessage(const std::string &name, T *object, Fun &&fun,
						  CommandExecutionQueue *executionQueue = nullptr,
						  CommandExecutionQueue *(*getExecutionQueue)(
							  MessageConverter *messageConverter, Peer *peer,
							  ByteReader &reader, Flags flags) = nullptr)
	{
		auto func = new MessageConverterSpecMethodOfObject(object, fun);
		func->_executionQueue = executionQueue;
		func->getExecutionQueue = getExecutionQueue;
		return RegisterAnyMessage(name, func);
	}

	inline MessageConverter *
	RegisterAnyMessage(const std::string &name,
					   MessageConverter *messageConverter)
	{
		registeredMessages[name] = messageConverter;
		return messageConverter;
	}

	template <typename... Targs>
	static void Send(Peer *peer, Flags flags, const std::string &name,
					 const Targs &...args)
	{
		ByteBuffer buffer(100);
		SerializeSend(buffer, flags, name, args...);
		peer->Send(std::move(buffer));
	}

	static void InitializeSerializeSend(ByteWriter &writer,
										const std::string &name)
	{
		writer.op(name);
	}

	static void FinalizeSerializeSend(ByteWriter &writer, Flags &flags)
	{
		flags |= FLAGS_CALL_NO_FEEDBACK;
		if (FramingProtocol::WriteHeaderIntoBuffer(writer._data, flags) ==
			false) {
			LOG_FATAL("Trying to write header into invalid ByteBuffer.");
		}
	}

	template <typename... Targs>
	static void SerializeSend(ByteBuffer &buffer, Flags flags,
							  const std::string &name, const Targs &...args)
	{
		ByteWriter writer(std::move(buffer));
		InitializeSerializeSend(writer, name);
		(writer.op(args), ...);
		FinalizeSerializeSend(writer, flags);
		buffer = std::move(writer._data);
	}

	template <typename... Targs>
	static ByteBuffer SerializeSend(Flags flags, const std::string &name,
									const Targs &...args)
	{
		ByteBuffer buffer(100);
		SerializeSend(buffer, flags, name, args...);
		return buffer;
	}

	template <typename... Targs>
	static void Call(CommandsBufferHandler *commandsBuffer, Peer *peer,
					 Flags flags, OnReturnCallback &&callback,
					 const std::string &name, const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op_untyped_uint32(0);
		writer.op(name);
		(writer.op(args), ...);

		commandsBuffer->EnqueueCommand<commands::internal::CommandCallSend>(
			peer->shared_from_this(), std::move(callback), flags,
			std::move(writer._data));
	}

	template <typename... Targs>
	static void Call(Peer *peer, Flags flags, OnReturnCallback &&callback,
					 const std::string &name, const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op_untyped_uint32(0);
		writer.op(name);
		(writer.op(args), ...);

		auto cb = CommandHandle<commands::internal::CommandCallSend>::Create(
			peer->shared_from_this(), std::move(callback), flags,
			std::move(writer._data));
		peer->host->EnqueueCommand(std::move(cb));
	}

	void RemoveRegisteredMessage(const std::string &name);

	friend class Host;

	static void CheckForTimeoutFunctionCalls(Loop *loop, uint32_t maxChecks);

protected:
	std::unordered_map<std::string, MessageConverter *> registeredMessages;
};
} // namespace icon7

#endif
