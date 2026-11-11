// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_WRITER_HPP
#define ICON7_BYTE_WRITER_HPP

#include "ByteBuffer.hpp"
#include "Forward.hpp" // IWYU pragma: export

#include "../../bitscpp/include/bitscpp/ByteWriter.hpp" // IWYU pragma: export

namespace icon7
{
extern bitscpp::GenericBufferWriterPimpl
	byte_buffer_writable_generic_wrapper_pimpl;

class ByteWriter : public bitscpp::ByteWriter
{
private:
	ByteBufferWritable _data;

public:
	~ByteWriter();
	ByteWriter();
	// 	ByteWriter(ByteBufferWritable &&buf);
	ByteWriter(ByteWriter &&o);
	ByteWriter(uint32_t initialCapacity);

	ByteWriter &operator=(ByteWriter &&o);

	ByteBufferReadable FinalizeAndReset();
	ByteBufferWritable ReleaseWritableBuffer();

	// 	uint8_t const *data();

	ByteWriter(ByteWriter &) = delete;
	ByteWriter(const ByteWriter &) = delete;
	ByteWriter &operator=(ByteWriter &) = delete;
	ByteWriter &operator=(const ByteWriter &) = delete;
};
} // namespace icon7

#endif
