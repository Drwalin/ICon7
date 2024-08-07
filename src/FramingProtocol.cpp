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

#include <cstdint>

#include "../include/icon7/Flags.hpp"
#include "../include/icon7/ByteBuffer.hpp"

#include "../include/icon7/FramingProtocol.hpp"

namespace icon7
{
uint32_t FramingProtocol::GetHeaderSize(uint32_t dataSize)
{
	if (dataSize <= 1 << 4) {
		return 1;
	} else if (dataSize <= 1 << 12) {
		return 2;
	} else if (dataSize <= 1 << 20) {
		return 3;
	} else if (dataSize <= 1 << 28) {
		return 4;
	} else {
		return 0;
	}
	return 0;
}

void FramingProtocol::WriteHeader(uint8_t *header, uint32_t headerSize,
								  uint32_t dataSize, Flags flags)
{
	uint32_t h = 0;
	h |= ((flags & 6) << 1);
	h |= (headerSize)-1;
	h |= (dataSize - 1) << 4;
	for (int i = 0; i < headerSize; ++i) {
		header[i] = (h >> (i << 3)) & 0xFF;
	}
}

uint32_t FramingProtocol::GetPacketHeaderSize(uint8_t headerFirstByte)
{
	return (headerFirstByte & 3) + 1;
}

Flags FramingProtocol::GetPacketFlags(uint8_t *header, Flags otherFlags)
{
	return otherFlags | Flags((header[0] >> 1) & 6);
}

uint32_t FramingProtocol::GetPacketBodySize(uint8_t *header, uint8_t headerSize)
{
	uint32_t h = 0;
	for (int i = 0; i < headerSize; ++i) {
		h |= ((uint32_t)header[i]) << (i << 3);
	}
	return (h >> 4) + 1;
}

[[nodiscard]] bool FramingProtocol::WriteHeaderIntoBuffer(ByteBuffer &buffer,
														  Flags flags)
{
	if (buffer.storage->offset != sizeof(ByteBufferStorageHeader) + 8 ||
		buffer.size() == 0) {
		return false;
	}

	memcpy(((uint8_t *)buffer.storage) + sizeof(ByteBufferStorageHeader),
		   &flags, 4);
	uint32_t headerSize = FramingProtocol::GetHeaderSize(buffer.size());
	uint8_t *header = buffer.data() - headerSize;
	FramingProtocol::WriteHeader(header, headerSize, buffer.size(), flags);
	buffer.storage->capacity += headerSize;
	buffer.storage->size += headerSize;
	buffer.storage->offset -= headerSize;
	return true;
}
} // namespace icon7
