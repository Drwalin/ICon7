// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_WRITER_HPP
#define ICON7_BYTE_WRITER_HPP

#include "ByteBuffer.hpp"
#include "Forward.hpp" // IWYU pragma: export

#include "../../bitscpp/include/bitscpp/ByteWriter_v2.hpp" // IWYU pragma: export

namespace bitscpp
{
namespace v2
{
extern template class ByteWriter<icon7::ByteBufferWritable>;
} // namespace v2
} // namespace bitscpp

namespace icon7
{
class ByteWriter : public ByteWriterBase
{
public:
	ByteBufferWritable _data;
	using Base = ByteWriterBase;

public:
	~ByteWriter() {}

	ByteWriter() {
		Init(&_data);
	}

	ByteWriter(ByteBufferWritable &&buf) : _data(std::move(buf))
	{
		Init(&_data);
	}

	ByteWriter(ByteWriter &&o) : _data(std::move(o._data))
	{
		Init(&_data);
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
