// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Flags.hpp"

#include "../include/icon7/PeerFlagsArgumentsReader.hpp"

namespace icon7
{
void PeerFlagsArgumentsReader::ReadType(CommandExecutionQueue *queue,
										PeerHandle peer, Flags flags,
										ByteReader &reader, Flags &value)
{
	value = flags;
}

void PeerFlagsArgumentsReader::ReadType(CommandExecutionQueue *queue,
										PeerHandle peer, Flags flags,
										ByteReader &reader, Host *&value)
{
	value = peer.GetHost();
}

void PeerFlagsArgumentsReader::ReadType(CommandExecutionQueue *queue,
										PeerHandle peer, Flags flags,
										ByteReader &reader, PeerHandle &value)
{
	value = peer;
}

void PeerFlagsArgumentsReader::ReadType(CommandExecutionQueue *queue,
										PeerHandle peer, Flags flags,
										ByteReader &reader, ByteReader *&value)
{
	value = &reader;
}

void PeerFlagsArgumentsReader::ReadType(CommandExecutionQueue *queue,
										PeerHandle peer, Flags flags,
										ByteReader &reader,
										CommandExecutionQueue *&value)
{
	value = queue;
}
} // namespace icon7
