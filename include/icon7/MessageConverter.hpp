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

#ifndef ICON7_MESSAGE_CONVERTER_HPP
#define ICON7_MESSAGE_CONVERTER_HPP

#include <tuple>

#include <bitscpp/ByteWriterExtensions.hpp>

#include "ByteReader.hpp"
#include "PeerFlagsArgumentsReader.hpp"
#include "Host.hpp"
#include "Peer.hpp"

namespace icon7
{

class RPCEnvironment;
class CommandExecutionQueue;

class MessageConverter
{
public:
	virtual ~MessageConverter() = default;

	virtual void Call(Peer *peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) = 0;

	CommandExecutionQueue *executionQueue;
};

template <typename Tret> class MessageReturnExecutor
{
public:
	template <typename TF, typename Tuple, size_t... SeqArgs>
	static void Execute(TF &&onReceive, Tuple &args, Peer *peer, Flags flags,
						uint32_t returnId, std::index_sequence<SeqArgs...>)
	{
		Tret ret = onReceive(std::get<SeqArgs>(args)...);
		if (returnId) {
			std::vector<uint8_t> buffer;
			{
				/*
				 * need this block, because writer resizes to correct size
				 * underlying buffer inside destructor
				 */
				bitscpp::ByteWriter writer(buffer);
				writer.op(returnId);
				writer.op(ret);
			}
			peer->Send(std::move(buffer), ((flags | Flags(6)) ^ Flags(6)) |
											  FLAGS_CALL_RETURN_FEEDBACK);
		}
	}
};

template <> class MessageReturnExecutor<void>
{
public:
	template <typename TF, typename Tuple, size_t... SeqArgs>
	static void Execute(TF &&onReceive, Tuple &args, Peer *peer, Flags flags,
						uint32_t returnId, std::index_sequence<SeqArgs...>)
	{
		onReceive(std::get<SeqArgs>(args)...);
		if (returnId) {
			std::vector<uint8_t> buffer;
			{
				/*
				 * need this block, because writer resizes to correct size
				 * underlying buffer inside destructor
				 */
				bitscpp::ByteWriter writer(buffer);
				writer.op(returnId);
			}
			peer->Send(std::move(buffer), ((flags | Flags(6)) ^ Flags(6)) |
											  FLAGS_CALL_RETURN_FEEDBACK);
		}
	}
};

template <typename Tret, typename... Targs>
class MessageConverterSpec : public MessageConverter
{
public:
	using TupleType = std::tuple<typename std::remove_const<
		typename std::remove_reference<Targs>::type>::type...>;

	MessageConverterSpec(Tret (*onReceive)(Targs... args))
		: onReceive(onReceive)
	{
	}

	virtual ~MessageConverterSpec() = default;

	virtual void Call(Peer *peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) override
	{
		auto seq = std::index_sequence_for<Targs...>{};
		_InternalCall(peer, flags, reader, returnId, seq);
	}

private:
	template <size_t... SeqArgs>
	void _InternalCall(Peer *peer, Flags flags, ByteReader &reader,
					   uint32_t returnId, std::index_sequence<SeqArgs...> seq)
	{
		TupleType args;
		(PeerFlagsArgumentsReader::ReadType(peer, flags, reader,
											std::get<SeqArgs>(args)),
		 ...);
		MessageReturnExecutor<Tret>::Execute(onReceive, args, peer, flags,
											 returnId, seq);
	}

private:
	Tret (*const onReceive)(Targs...);
};

} // namespace icon7

#endif
