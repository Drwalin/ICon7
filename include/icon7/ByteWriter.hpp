// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_WRITER_HPP
#define ICON7_BYTE_WRITER_HPP

#include "ByteBuffer.hpp"

#undef BITSCPP_BYTE_WRITER_V2_BT_TYPE
#define BITSCPP_BYTE_WRITER_V2_BT_TYPE icon7::ByteBufferWritable

#undef BITSCPP_BYTE_WRITER_V2_NAME_SUFFIX
#define BITSCPP_BYTE_WRITER_V2_NAME_SUFFIX _ByteBuffer

#include "../../bitscpp/include/bitscpp/ByteWriter_v2.hpp" // IWYU pragma: export

namespace icon7
{
static_assert(std::is_same_v<BITSCPP_BYTE_WRITER_V2_BT_TYPE, icon7::ByteBufferWritable>);
	
class ByteWriter : public bitscpp::v2::ByteWriter_ByteBuffer
{
public:
	ByteBufferWritable _data;
	using Base = bitscpp::v2::ByteWriter_ByteBuffer;

public:
	~ByteWriter() {};

	ByteWriter(ByteBufferWritable &&buf) : _data(std::move(buf))
	{
	}

	ByteWriter(ByteWriter &&o) : _data(std::move(o._data))
	{
	}

	inline ByteBufferWritable &Buffer() { return _data; }

	ByteWriter(uint32_t initialCapacity) : _data(initialCapacity)
	{
		Init(&_data);
	}

	void Reinit(uint32_t capacity)
	{
		_data.clear();
		_data.reserve(capacity);
		Init(&_data);
	}

	inline ByteWriter &operator=(ByteWriter &&o)
	{
		this->~ByteWriter();
		new (this) ByteWriter(std::move(o));
		return *this;
	}

	inline uint8_t const *data() { return _data.data(); }

	ByteWriter(ByteWriter &) = delete;
	ByteWriter(const ByteWriter &) = delete;
	ByteWriter &operator=(ByteWriter &) = delete;
	ByteWriter &operator=(const ByteWriter &) = delete;
};
} // namespace icon7

#endif
