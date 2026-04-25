// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdint>

#include "../include/icon7/Flags.hpp"
#include "../include/icon7/ByteBuffer.hpp"
#include "../include/icon7/Debug.hpp"
#include "../bitscpp/include/bitscpp/ByteReader_v2.hpp"

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

uint32_t FramingProtocol::GetPacketBodySize(const uint8_t *header,
											uint8_t headerSize)
{
	uint32_t h = 0;
	for (int i = 0; i < headerSize; ++i) {
		h |= ((uint32_t)header[i]) << (i << 3);
	}
	return (h >> 4) + 1;
}

[[nodiscard]] bool
FramingProtocol::WriteHeaderIntoBuffer(ByteBufferWritable &buffer, Flags flags)
{
	if (buffer._offset < sizeof(ByteBufferStorageHeader) + 4 ||
		buffer.size() == 0) {
		LOG_FATAL("Error; invalid buffer: size=%u  offset: %u  cap: %u",
				  buffer._size, buffer._offset, buffer._capacity);
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

void FramingProtocol::PrintDetailsAboutFrame(ByteBufferReadable &frame)
{
	const uint32_t totalSize = frame.size();
	const uint32_t headerSize = GetPacketHeaderSize(frame.data()[0]);
	uint32_t header;
	memcpy(&header, frame.data(), headerSize);
	const uint32_t calculatedBodySize = totalSize - headerSize;
	const uint32_t storedInFrameBodySize =
		GetPacketBodySize(frame.data(), headerSize);
	const Flags flagsStored = frame.storage->flags;
	const Flags flagsRead =
		GetPacketFlags(frame.data(), flagsStored & FLAG_RELIABLE);

	char str[1024];
	memset(str, 0, 1024);
	char *ptr = str;
	const char *end = str+1023;

	assert(storedInFrameBodySize == calculatedBodySize);
	assert(flagsStored == flagsRead);

	ptr += snprintf(ptr, end - ptr,
			"totalSize: %i   "
			"headerSize: %i   "
			"storedBodySize: %i   "
			"calculatedBodySize: %i   "
			"flagsStored: %X   "
			"flagsRead: %X   ",
			totalSize,
			headerSize,
			storedInFrameBodySize,
			calculatedBodySize,
			flagsStored,
			flagsRead
			);

	bitscpp::v2::ByteReader reader(frame.data(), headerSize, totalSize);

	uint32_t callId;
	std::string name;

	switch (flagsRead & 6) {
	case FLAGS_CALL:
		reader.op_untyped_uint32(callId);
		reader.op(name);
		ptr += snprintf(ptr, end - ptr,
				"CALL (%u): `%s`   ",
				callId, name.c_str()
				);
		break;
	case FLAGS_CALL_NO_FEEDBACK:
		reader.op(name);
		ptr += snprintf(ptr, end - ptr,
				"SEND: `%s`   ",
				name.c_str()
				);
		break;
	case FLAGS_CALL_RETURN_FEEDBACK:
		reader.op_untyped_uint32(callId);
		ptr += snprintf(ptr, end - ptr,
				"RETURN (%u)   ",
				callId
				);
		break;
	case FLAGS_PROTOCOL_CONTROLL_SEQUENCE:
		callId = frame.data()[headerSize];
		ptr += snprintf(ptr, end - ptr,
				"CONTROL SEQUENCE: %u   ",
				callId
				);
		break;
	}

	assert(reader.get_errors() == 0);

	LOG_TRACE("%s", str);
}
} // namespace icon7
