// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <bit>

#include "../include/icon7/MemoryPool.hpp"
#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/ByteWriter.hpp"

#include "../include/icon7/ByteBuffer.hpp"

namespace icon7
{
void PrintBufferMetadata(ByteBufferStorageHeader *buffer)
{
	return;
	const uint8_t *data = buffer->data();
	const uint32_t size = buffer->size;
	const uint32_t headerSize = FramingProtocol::GetPacketHeaderSize(data[0]);
	const uint32_t bodySize = FramingProtocol::GetPacketBodySize(data, headerSize);
	const Flags flags = FramingProtocol::GetPacketFlags(data, 0);
	printf("buffer [%2u],  h[%2u] = {", size, headerSize);
	for (int i=0; i<headerSize; ++i) {
		printf("%s%2.2x", i?" ":"", (uint32_t)data[i]);
	}
	printf("}  ");

	std::string name;
	uint32_t callId = 0;
	bitscpp::v2::ByteReader reader(data, headerSize, size);
	switch(flags & 6) {
		case FLAGS_CALL_NO_FEEDBACK:
			reader.op(name);
			printf("invoke `%s`[%lu]", name.c_str(), name.size());
			break;
		case FLAGS_CALL:
			reader.op_untyped_uint32(callId);
			reader.op(name);
			printf("call `%s`[%lu]->%u", name.c_str(), name.size(), callId);
			break;
		case FLAGS_CALL_RETURN_FEEDBACK:
			reader.op_untyped_uint32(callId);
			printf("return %u", callId);
			break;
		case FLAGS_PROTOCOL_CONTROLL_SEQUENCE:
			printf("ctrl: %u", data[headerSize]);
			reader.skip(1);
			break;
	}
	printf("  data [%u]:", reader.get_remaining_bytes());

	for (uint32_t i=reader.get_offset(); i<size; ++i) {
		printf(" %2.2X", (uint32_t)data[i]);
	}
	printf("\n");
	fflush(stdout);
}
}

#include "../../ICon7/bitscpp/src/ByteWriter_v2.cpp"
