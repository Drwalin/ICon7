// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <bit> // IWYU pragma: keep

#include "../include/icon7/MemoryPool.hpp" // IWYU pragma: keep
#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/ByteWriter.hpp" // IWYU pragma: keep

#include "../include/icon7/ByteBuffer.hpp"

namespace icon7
{
void PrintBufferMetadata(ByteBufferStorageHeader *buffer)
{
	return;
	const uint8_t *data = buffer->data();
	const uint32_t size = buffer->size;
	const uint32_t headerSize = FramingProtocol::GetPacketHeaderSize(data[0]);
	const Flags flags = FramingProtocol::GetPacketFlags(data, 0);
	printf("buffer [%2u],  h[%2u] = {", size, headerSize);
	for (int i = 0; i < headerSize; ++i) {
		printf("%s%2.2x", i ? " " : "", (uint32_t)data[i]);
	}
	printf("}  ");

	std::string name;
	uint32_t callId = 0;
	bitscpp::v2::ByteReader reader(data, headerSize, size);
	switch (flags & 6) {
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

	for (uint32_t i = reader.get_offset(); i < size; ++i) {
		printf(" %2.2X", (uint32_t)data[i]);
	}
	printf("\n");
	fflush(stdout);
}

void ByteBufferStorageHeader::free(ByteBufferStorageHeader *header)
{
	assert(header->refCounter == 0);
	MemoryPool::Release(header, header->capacity + header->offset);
}

ByteBufferWritable::ByteBufferWritable(const ByteBufferWritable &o)
	: ByteBufferWritable()
{
	if (o.buffer != nullptr) {
		new (this) ByteBufferWritable(o._size);
		append(o.data(), o.size());
		_flags = o._flags;
	}
}
ByteBufferWritable::ByteBufferWritable(ByteBufferWritable &o)
	: ByteBufferWritable((const ByteBufferWritable &)o)
{
}
ByteBufferWritable::ByteBufferWritable(uint32_t capacity) : ByteBufferWritable()
{
	if (capacity == 0) {
		return;
	}
	_offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
	capacity = round_capacity_up(capacity);
	_size = 0;
	_flags = 0;
	auto buf = MemoryPool::Allocate(capacity + _offset);
	buffer = ((uint8_t *)buf.object) + _offset;
	_capacity = buf.capacity - _offset;
}

void ByteBufferWritable::free_buffer()
{
	if (buffer) {
		assert(_capacity);
		assert(_offset);
		MemoryPool::Release(buffer - _offset, _capacity + _offset);
		buffer = nullptr;
		_size = 0;
		_capacity = 0;
		_offset = 0;
		_flags = 0;
	}
}

void ByteBufferWritable::reserve_or_shrink(const uint32_t minCapacity)
{
	assert(_size <= _capacity);
	const uint32_t capacity = round_capacity_up(minCapacity);
	ByteBufferWritable buf(capacity);
	buf._flags = _flags;
	if (buffer != nullptr) {
		buf.append(buffer, std::min(_size, buf._capacity));
	}
	*this = std::move(buf);
	assert(buffer);
}

uint32_t ByteBufferWritable::round_capacity_up(uint32_t capacity)
{
	assert(capacity < (1 << 30));
	capacity += INITIAL_DATA_OFFSET;
	const size_t width = std::bit_width(std::max(capacity, 64u));
	uint32_t maxCap = 1 << width;
	const uint32_t diff = 1 << (width - 2);
	while (maxCap >= capacity) {
		maxCap -= diff;
	}
	maxCap += diff;
	return maxCap - INITIAL_DATA_OFFSET;
}
} // namespace icon7

#include "../../ICon7/bitscpp/src/ByteWriter_v2.cpp"
