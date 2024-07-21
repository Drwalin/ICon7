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

#ifndef ICON7_FRAMING_PROTOCOL_HPP
#define ICON7_FRAMING_PROTOCOL_HPP

#include <cstdint>

#include "Flags.hpp"

namespace icon7
{
class ByteBuffer;

class FramingProtocol
{
public:
	static uint32_t GetHeaderSize(uint32_t dataSize);
	static void WriteHeader(uint8_t *header, uint32_t headerSize,
							uint32_t dataSize, Flags flags);
	static uint32_t GetPacketHeaderSize(uint8_t headerFirstByte);
	static Flags GetPacketFlags(uint8_t *header, Flags otherFlags);
	static uint32_t GetPacketBodySize(uint8_t *header, uint8_t headerSize);

	/*
	 * returns false for invalid ByteBuffer object
	 */
	[[nodiscard]] static bool WriteHeaderIntoBuffer(ByteBuffer &buffer,
													Flags flags);
};
} // namespace icon7

#endif
