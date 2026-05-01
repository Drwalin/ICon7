// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_MESSAGE_CONVERTER_HPP
#define ICON7_MESSAGE_CONVERTER_HPP

#include <tuple>

#include "ByteWriter.hpp"

#include "Debug.hpp"
#include "ByteReader.hpp"
#include "PeerFlagsArgumentsReader.hpp"
#include "Peer.hpp"
#include "FramingProtocol.hpp"

namespace icon7
{

class RPCEnvironment;
class CommandExecutionQueue;

class MessageConverter
{
public:
	MessageConverter()
	{
		getExecutionQueue = nullptr;
		_executionQueue = nullptr;
	}
	virtual ~MessageConverter() {}

	virtual void Call(PeerHandle peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) = 0;

	inline CommandExecutionQueue *
	ExecuteGetQueue(PeerHandle peer, ByteReader &reader, Flags flags)
	{
		if (_executionQueue) {
			return _executionQueue;
		}
		if (getExecutionQueue) {
			return getExecutionQueue(this, peer, reader, flags);
		}
		return nullptr;
	}

	CommandExecutionQueue *(*getExecutionQueue)(
		MessageConverter *messageConverter, PeerHandle peer, ByteReader &reader,
		Flags flags);

	CommandExecutionQueue *_executionQueue;
};

template <typename Tret> class MessageReturnExecutor
{
public:
	template <typename TF, typename Tuple, size_t... SeqArgs>
	static void Execute(TF &&onReceive, Tuple &args, PeerHandle peer, Flags flags,
						uint32_t returnId, std::index_sequence<SeqArgs...>)
	{
		Tret ret = onReceive(std::get<SeqArgs>(args)...);
		if (returnId && ((flags & 6) == FLAGS_CALL)) {
			ByteWriter writer(100);
			writer.op_untyped_uint32(returnId);
			writer.op(ret);
			if (FramingProtocol::WriteHeaderIntoBuffer(
					writer._data, ((flags | Flags(6)) ^ Flags(6)) |
									  FLAGS_CALL_RETURN_FEEDBACK)) {
				peer.GetLocalPeerData()->Send(ByteBufferReadable(std::move(writer._data)));
			} else {
				LOG_FATAL("Trying to write header into invalid ByteBuffer.");
			}
		} else if (returnId) {
			LOG_WARN(
				"It should never happen -> it's a bug, where MessegeConverter "
				"receives non 0 returnId for non returning RPC send.");
		}
	}
};

template <> class MessageReturnExecutor<void>
{
public:
	template <typename TF, typename Tuple, size_t... SeqArgs>
	static void Execute(TF &&onReceive, Tuple &args, PeerHandle peer, Flags flags,
						uint32_t returnId, std::index_sequence<SeqArgs...>)
	{
		onReceive(std::get<SeqArgs>(args)...);
		if (returnId && ((flags & 6) == FLAGS_CALL)) {
			ByteWriter writer(100);
			writer.op_untyped_uint32(returnId);
			if (FramingProtocol::WriteHeaderIntoBuffer(
					writer._data, ((flags | Flags(6)) ^ Flags(6)) |
									  FLAGS_CALL_RETURN_FEEDBACK)) {
				peer.GetLocalPeerData()->Send(ByteBufferReadable(std::move(writer._data)));
			} else {
				LOG_FATAL("Trying to write header into invalid ByteBuffer.");
			}
		} else if (returnId) {
			LOG_WARN(
				"It should never happen -> it's a bug, where MessegeConverter "
				"receives non 0 returnId for non returning RPC send.");
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

	virtual ~MessageConverterSpec() {}

	virtual void Call(PeerHandle peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) override
	{
		auto seq = std::index_sequence_for<Targs...>{};
		_InternalCall(peer, flags, reader, returnId, seq);
	}

private:
	template <size_t... SeqArgs>
	void _InternalCall(PeerHandle peer, Flags flags, ByteReader &reader,
					   uint32_t returnId, std::index_sequence<SeqArgs...> seq)
	{
		TupleType args;
		(PeerFlagsArgumentsReader::ReadType(peer, flags, reader,
											std::get<SeqArgs>(args)),
		 ...);
		if (reader.get_errors() != 0) {
			LOG_ERROR("Failed to execute function call, error in ByteReader.");
		} else {
			MessageReturnExecutor<Tret>::Execute(onReceive, args, peer, flags,
												 returnId, seq);
		}
	}

private:
	Tret (*const onReceive)(Targs...);
};

template <typename Tret, typename... Targs>
class MessageConverterSpecStdFunction : public MessageConverter
{
public:
	using TupleType = std::tuple<typename std::remove_const<
		typename std::remove_reference<Targs>::type>::type...>;

	MessageConverterSpecStdFunction(
		std::function<Tret(Targs... args)> onReceive)
		: onReceive(onReceive)
	{
	}

	virtual ~MessageConverterSpecStdFunction() {}

	virtual void Call(PeerHandle peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) override
	{
		auto seq = std::index_sequence_for<Targs...>{};
		_InternalCall(peer, flags, reader, returnId, seq);
	}

private:
	template <size_t... SeqArgs>
	void _InternalCall(PeerHandle peer, Flags flags, ByteReader &reader,
					   uint32_t returnId, std::index_sequence<SeqArgs...> seq)
	{
		TupleType args;
		(PeerFlagsArgumentsReader::ReadType(peer, flags, reader,
											std::get<SeqArgs>(args)),
		 ...);
		if (reader.get_errors() != 0) {
			LOG_ERROR("Failed to execute function call, error in ByteReader.");
		} else {
			MessageReturnExecutor<Tret>::Execute(onReceive, args, peer, flags,
												 returnId, seq);
		}
	}

private:
	std::function<Tret(Targs... args)> onReceive;
};

template <typename T, typename Tret, typename... Targs>
class MessageConverterSpecMethodOfObject : public MessageConverter
{
public:
	using TupleType = std::tuple<typename std::remove_const<
		typename std::remove_reference<Targs>::type>::type...>;

	MessageConverterSpecMethodOfObject(T *object,
									   Tret (T::*onReceive)(Targs... args))
		: object(object), onReceive(onReceive)
	{
	}

	virtual ~MessageConverterSpecMethodOfObject() {}

	virtual void Call(PeerHandle peer, ByteReader &reader, Flags flags,
					  uint32_t returnId) override
	{
		auto seq = std::index_sequence_for<Targs...>{};
		_InternalCall(peer, flags, reader, returnId, seq);
	}

private:
	template <size_t... SeqArgs>
	void _InternalCall(PeerHandle peer, Flags flags, ByteReader &reader,
					   uint32_t returnId, std::index_sequence<SeqArgs...> seq)
	{
		TupleType args;
		(PeerFlagsArgumentsReader::ReadType(peer, flags, reader,
											std::get<SeqArgs>(args)),
		 ...);
		if (reader.get_errors() != 0) {
			LOG_ERROR("Failed to execute function call, error in ByteReader.");
		} else {
			MessageReturnExecutor<Tret>::Execute(
				[this](Targs... args) -> Tret {
					return (object->*onReceive)(args...);
				},
				args, peer, flags, returnId, seq);
		}
	}

private:
	T *object;
	Tret (T::*onReceive)(Targs... args);
};

} // namespace icon7

#endif
