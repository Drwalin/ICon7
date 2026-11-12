// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../drwalin_util/include/drwalin_util/assert.hpp"
#include "../include/icon7/ByteWriter.hpp"

namespace icon7
{
bitscpp::GenericBufferWriterPimpl byte_buffer_writable_generic_wrapper_pimpl = {
	.expand =
		+[](bitscpp::GenericBufferWriter *buffer, uint8_t minimum,
			std::size_t hint) {
			using TB = ByteBufferWritable;
			TB *buf = (TB *)(buffer->buffer);
			ASSERT_EXIT(buf != nullptr);
			ptrdiff_t offset = 0;
			if (buffer->head) {
				offset = buffer->head - buf->data();
			}
			size_t new_size = minimum + hint + buf->size() + 16;
			if (new_size < buf->capacity()) {
				new_size = buf->capacity();
			}
			ASSERT_DEBUG(
				(buf->data() == nullptr && buffer->head == nullptr &&
				 buffer->end == nullptr) ||
				(buf->data() != nullptr &&
				 ((buffer->head == buffer->end) ||
				  (buffer->head < buffer->end && buffer->head != nullptr))));
			new_size = (new_size * 3) / 2;
			buf->resize(new_size);
			buffer->head = buf->data() + offset;
			buffer->end = buf->data() + buf->size();
		},

	.destructor =
		+[](bitscpp::GenericBufferWriter *buffer) {
			ASSERT_DEBUG(buffer->head == nullptr);
			ASSERT_DEBUG(buffer->end == nullptr);
		},

	.apply_head_end_to_underlying =
		+[](bitscpp::GenericBufferWriter *buffer) {
			using TB = ByteBufferWritable;
			TB *buf = (TB *)(buffer->buffer);
			ASSERT_EXIT(buf != nullptr);
			ptrdiff_t offset = 0;
			if (buffer->head) {
				offset = buffer->head - buf->data();
			}
			ASSERT_DEBUG(
				(buf->data() == nullptr && buffer->head == nullptr &&
				 buffer->end == nullptr) ||
				(buf->data() != nullptr &&
				 ((buffer->head == buffer->end) ||
				  (buffer->head < buffer->end && buffer->head != nullptr))));
			buf->resize(offset);
			buffer->head = buf->data() + offset;
			buffer->end = buf->data() + buf->size();
		}};

ByteWriter::~ByteWriter() {}

ByteWriter::ByteWriter()
	: bitscpp::ByteWriter(bitscpp::GenericBufferWriter(
		  &_data, byte_buffer_writable_generic_wrapper_pimpl))
{
}

ByteWriter::ByteWriter(ByteWriter &&o)
	: bitscpp::ByteWriter(std::move((bitscpp::ByteWriter &&)o)),
	  _data(std::move(o._data))
{
	_buffer.buffer = &_data;
}

ByteWriter::ByteWriter(uint32_t initialCapacity) : ByteWriter()
{
	_data.reserve(initialCapacity);
}

ByteBufferReadable ByteWriter::FinalizeAndReset()
{
	ByteBufferWritable *b = (ByteBufferWritable *)release_buffer();
	ASSERT_EXIT(b == &_data);
	ByteBufferReadable ret(std::move(_data));
	_buffer.buffer = &_data;
	return ret;
}

ByteBufferWritable ByteWriter::ReleaseWritableBuffer()
{
	ASSERT_EXIT(_buffer.buffer == &_data);
	void *_ret = release_buffer();
	ASSERT_EXIT(_ret == &_data);
	ByteBufferWritable *ret = (ByteBufferWritable *)_ret;
	_buffer.buffer = &_data;
	return std::move(*ret);
}

ByteWriter &ByteWriter::operator=(ByteWriter &&o)
{
	this->~ByteWriter();
	new (this) ByteWriter(std::move(o));
	return *this;
}

// uint8_t const ByteWriter::*data() { return _data.data(); }
} // namespace icon7
