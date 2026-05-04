// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_READER_HPP
#define ICON7_BYTE_READER_HPP

#include "../../bitscpp/include/bitscpp/ByteReader_v2.hpp" // IWYU pragma: export

#include "ByteBuffer.hpp"

namespace icon7
{
class ByteReader : public bitscpp::v2::ByteReader
{
public:
	ByteBufferReadable _data;

public:
	inline ByteReader() : bitscpp::v2::ByteReader(nullptr, 0, 0) {}

	inline ByteReader(ByteBufferReadable &data, uint32_t offset)
		: bitscpp::v2::ByteReader(data.data(), offset, data.size()),
		  _data(data)
	{
	}

	inline ByteReader(ByteBufferReadable &&data, uint32_t offset)
		: bitscpp::v2::ByteReader(data.data(), offset, data.size()),
		  _data(std::move(data))
	{
	}

	inline ~ByteReader() {}

	inline ByteReader(ByteReader &&o)
		: bitscpp::v2::ByteReader(o._buffer, o.get_offset(), o.total_size),
		  _data(std::move(o._data))
	{
		errors = o.errors;
	}

	inline ByteReader &operator=(ByteReader &&o)
	{
		this->~ByteReader();
		new (this) ByteReader(std::move(o));
		return *this;
	}

	inline uint8_t const *data() { return _buffer; }

	ByteReader(ByteReader &) = delete;
	ByteReader(const ByteReader &) = delete;
	ByteReader &operator=(ByteReader &) = delete;
	ByteReader &operator=(const ByteReader &) = delete;
};
} // namespace icon7

#endif
