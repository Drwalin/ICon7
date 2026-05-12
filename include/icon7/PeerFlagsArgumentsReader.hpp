// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_PEER_FLAGS_ARGUMENTS_READER_HPP
#define ICON7_PEER_FLAGS_ARGUMENTS_READER_HPP

#include "ByteReader.hpp"
#include "Flags.hpp"
#include "Peer.hpp"

namespace icon7
{
class Peer;
class Host;

class PeerFlagsArgumentsReader
{
public:
	static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
						 Flags flags, ByteReader &reader, Flags &v);
	static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
						 Flags flags, ByteReader &reader, Host *&v);
	static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
						 Flags flags, ByteReader &reader, PeerHandle &v);
	static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
						 Flags flags, ByteReader &reader, ByteReader *&v);
	static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
						 Flags flags, ByteReader &reader,
						 CommandExecutionQueue *&v);

	template <typename T>
	inline static void ReadType(CommandExecutionQueue *queue, PeerHandle peer,
								Flags flags, ByteReader &reader, T &value)
	{
		reader.op(value);
	}
};
} // namespace icon7

#endif
