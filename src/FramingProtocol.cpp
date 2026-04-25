// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdint>

#include "../include/icon7/Flags.hpp"
#include "../include/icon7/ByteBuffer.hpp"
#include "../include/icon7/Debug.hpp"
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

Flags FramingProtocol::GetPacketFlags(const uint8_t *header, Flags otherFlags)
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

[[nodiscard]] bool FramingProtocol::WriteHeaderIntoBuffer(ByteBufferWritable &buffer,
														  Flags flags)
{
	if (buffer._offset < sizeof(ByteBufferStorageHeader) + 4 ||
		buffer.size() == 0) {
		LOG_FATAL("Error; buffer: size=%u  offset: %u  cap: %u",
				buffer._size,
				buffer._offset,
				buffer._capacity
				);
		return false;
	}

	buffer._flags = flags;
	uint32_t headerSize = FramingProtocol::GetHeaderSize(buffer.size());
	uint8_t *header = buffer.data() - headerSize;
	FramingProtocol::WriteHeader(header, headerSize, buffer.size(), flags);
	buffer._capacity += headerSize;
	buffer._size += headerSize;
	buffer._offset -= headerSize;
	buffer.buffer -= headerSize;
	return true;
}
} // namespace icon7
