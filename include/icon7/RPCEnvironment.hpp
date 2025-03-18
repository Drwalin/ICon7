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

#ifndef ICON7_RPC_ENVIRONMENT_HPP
#define ICON7_RPC_ENVIRONMENT_HPP

#include <string>
#include <unordered_map>
#include <vector>

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

namespace icon7
{
class CommandExecutionQueue;

class RPCEnvironment
{
public:
	RPCEnvironment();
	~RPCEnvironment();

	void OnReceive(Peer *peer, ByteBuffer &frameData, uint32_t headerSize,
				   Flags flags);
	/*
	 * expects that reader already filled flags fully and reader is at body
	 * offset
	 */
	void OnReceive(Peer *peer, ByteReader &reader, Flags flags);
	void OnReceiveCall(Peer *peer, ByteReader &reader, Flags flags);
	void OnReceiveReturn(Peer *peer, ByteReader &reader, Flags flags);

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
	void Send(Peer *peer, Flags flags, const std::string &name,
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

	class CommandCallSend final : public commands::ExecuteOnPeer
	{
	public:
		CommandCallSend(std::shared_ptr<Peer> peer, RPCEnvironment *rpcEnv,
						OnReturnCallback callback, Flags flags,
						ByteBuffer buffer)
			: ExecuteOnPeer(peer), rpcEnv(rpcEnv),
			  callback(std::move(callback)), flags(flags), buffer(buffer)
		{
		}
		virtual ~CommandCallSend() {}

		RPCEnvironment *rpcEnv;
		OnReturnCallback callback;
		Flags flags;
		ByteBuffer buffer;

		virtual void Execute() override
		{
			const uint32_t rcbId = rpcEnv->GetNewReturnIdCallback(peer.get());
			const uint32_t rcbId_v =
				bitscpp::HostToNetworkUint<uint32_t>(rcbId);
			memcpy(buffer.data(), &rcbId_v, sizeof(rcbId_v));
			rpcEnv->returningCallbacks[rcbId][peer.get()] = std::move(callback);
			flags |= FLAGS_CALL;
			if (FramingProtocol::WriteHeaderIntoBuffer(buffer, flags)) {
				peer->SendLocalThread(std::move(buffer));
			} else {
				LOG_FATAL("Trying to write header into invalid ByteBuffer.");
			}
		}
	};

	template <typename... Targs>
	void Call(CommandsBufferHandler *commandsBuffer, Peer *peer, Flags flags,
			  OnReturnCallback &&callback, const std::string &name,
			  const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op((uint32_t)0);
		writer.op(name);
		(writer.op(args), ...);

		commandsBuffer->EnqueueCommand<CommandCallSend>(
			peer->shared_from_this(), this, std::move(callback), flags,
			writer._data);
	}

	template <typename... Targs>
	void Call(Peer *peer, Flags flags, OnReturnCallback &&callback,
			  const std::string &name, const Targs &...args)
	{
		ByteWriter writer(100);
		writer.op((uint32_t)0);
		writer.op(name);
		(writer.op(args), ...);

		auto cb = CommandHandle<CommandCallSend>::Create(
			peer->shared_from_this(), this, std::move(callback), flags,
			writer._data);
		host->EnqueueCommand(std::move(cb));
	}

	void CheckForTimeoutFunctionCalls(uint32_t maxChecks = 10);

	void RemoveRegisteredMessage(const std::string &name);

	uint32_t GetNewReturnIdCallback(Peer *peer);

	friend class Host;

protected:
	std::unordered_map<std::string, MessageConverter *> registeredMessages;
	Host *host = nullptr;

	std::unordered_map<uint32_t, std::unordered_map<Peer *, OnReturnCallback>>
		returningCallbacks;
	uint32_t lastCheckedId = 1;

	std::vector<OnReturnCallback> timeouts;
};
} // namespace icon7

#endif
