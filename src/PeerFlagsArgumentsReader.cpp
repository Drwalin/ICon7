// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Flags.hpp"
#include "../include/icon7/Host.hpp"

#include "../include/icon7/PeerFlagsArgumentsReader.hpp"

namespace icon7
{
void PeerFlagsArgumentsReader::ReadType(Peer *peer, Flags flags,
										ByteReader &reader, Flags &value)
{
	value = flags;
}

void PeerFlagsArgumentsReader::ReadType(Peer *peer, Flags flags,
										ByteReader &reader, Host *&value)
{
	value = peer->host;
}

void PeerFlagsArgumentsReader::ReadType(Peer *peer, Flags flags,
										ByteReader &reader, Peer *&value)
{
	value = peer;
}

void PeerFlagsArgumentsReader::ReadType(Peer *peer, Flags flags,
										ByteReader &reader, ByteReader *&value)
{
	value = &reader;
}
} // namespace icon7
