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
	static void ReadType(Peer *peer, Flags flags, ByteReader &reader, Flags &v);
	static void ReadType(Peer *peer, Flags flags, ByteReader &reader, Host *&v);
	static void ReadType(Peer *peer, Flags flags, ByteReader &reader, Peer *&v);
	static void ReadType(Peer *peer, Flags flags, ByteReader &reader,
						 ByteReader *&v);

	template <typename T>
	inline static void ReadType(Peer *peer, Flags flags, ByteReader &reader,
								T &value)
	{
		reader.op(value);
	}
};
} // namespace icon7

#endif
