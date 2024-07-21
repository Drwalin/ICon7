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
