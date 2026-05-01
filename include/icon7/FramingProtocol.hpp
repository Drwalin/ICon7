// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FRAMING_PROTOCOL_HPP
#define ICON7_FRAMING_PROTOCOL_HPP

#include <cstdint>

#include "Flags.hpp"

namespace icon7
{
class ByteBufferWritable;
class ByteBufferReadable;

class FramingProtocol
{
public:
	static uint32_t GetHeaderSize(uint32_t dataSize);
	static void WriteHeader(uint8_t *header, uint32_t headerSize,
							uint32_t dataSize, Flags flags);
	static uint32_t GetPacketHeaderSize(uint8_t headerFirstByte);
	static Flags GetPacketFlags(const uint8_t *header, Flags otherFlags);
	static uint32_t GetPacketBodySize(const uint8_t *header,
									  uint8_t headerSize);

	/*
	 * returns false for invalid ByteBufferWritable object
	 */
	[[nodiscard]] static bool WriteHeaderIntoBuffer(ByteBufferWritable &buffer,
													Flags flags);
	static void PrintDetailsAboutFrame(const ByteBufferReadable &frame);
};
} // namespace icon7

#endif
