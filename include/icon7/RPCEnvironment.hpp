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

#include <bitscpp/ByteWriter.hpp>

#include "Util.hpp"
#include "Flags.hpp"
#include "Host.hpp"
#include "Peer.hpp"
#include "OnReturnCallback.hpp"
#include "MessageConverter.hpp"

namespace icon7
{

class CommandExecutionQueue;

class RPCEnvironment
{
public:
	~RPCEnvironment();

	void OnReceive(Peer *peer, std::vector<uint8_t> &frameData,
				   uint32_t headerSize, Flags flags);
	/*
	 * expects that reader already filled flags fully and reader is at body
	 * offset
	 */
	void OnReceive(Peer *peer, ByteReader &reader, Flags flags);

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
		std::vector<uint8_t> buffer;
		bitscpp::ByteWriter writer(buffer);
		SerializeSend(writer, flags, name, args...);
		peer->Send(std::move(buffer), flags);
	}

	template <typename... Targs>
	static void SerializeSend(bitscpp::ByteWriter &writer, Flags &flags,
							  const std::string &name, const Targs &...args)
	{
		writer.op(name);
		(writer.op(args), ...);
		flags |= FLAGS_CALL_NO_FEEDBACK;
	}

	template <typename... Targs>
	void Call(Peer *peer, Flags flags, OnReturnCallback &&callback,
			  const std::string &name, const Targs &...args)
	{
		uint32_t returnCallbackId = 0;
		{
			std::lock_guard guard{mutexReturningCallbacks};
			do {
				returnCallbackId = ++returnCallCallbackIdGenerator;
			} while (returnCallbackId == 0 &&
					 returningCallbacks.count(returnCallbackId) != 0);
			returningCallbacks[returnCallbackId] = std::move(callback);
		}
		std::vector<uint8_t> buffer;
		bitscpp::ByteWriter writer(buffer);
		writer.op(returnCallbackId);
		writer.op(name);
		(writer.op(args), ...);
		peer->Send(std::move(buffer), flags | FLAGS_CALL);
	}

	void CheckForTimeoutFunctionCalls(uint32_t maxChecks = 10);

	void RemoveRegisteredMessage(const std::string &name);

protected:
	std::unordered_map<std::string, MessageConverter *> registeredMessages;

	std::mutex mutexReturningCallbacks;
	uint32_t returnCallCallbackIdGenerator = 0;
	std::unordered_map<uint32_t, OnReturnCallback> returningCallbacks;
	uint32_t lastCheckedId = 1;
};
} // namespace icon7

#endif
