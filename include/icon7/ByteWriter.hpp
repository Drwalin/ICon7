// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_WRITER_HPP
#define ICON7_BYTE_WRITER_HPP

#include "ByteBuffer.hpp"

#ifndef BITSCPP_BYTE_WRITER_V2_BT_TYPE
#define BITSCPP_BYTE_WRITER_V2_BT_TYPE icon7::ByteBuffer
#endif

#ifndef BITSCPP_BYTE_WRITER_V2_NAME_SUFFIX
#define BITSCPP_BYTE_WRITER_V2_NAME_SUFFIX _ByteBuffer
#endif

#include "../../bitscpp/include/bitscpp/ByteWriter_v2.hpp"

namespace icon7
{
static_assert(std::is_same_v<BITSCPP_BYTE_WRITER_V2_BT_TYPE, icon7::ByteBuffer>);
	
class ByteWriter : public bitscpp::v2::ByteWriter_ByteBuffer
{
public:
	ByteBuffer _data;
	using Base = bitscpp::v2::ByteWriter_ByteBuffer;

public:
	~ByteWriter() {};

	ByteWriter(ByteBuffer &&buf) : Base(&_data), _data(std::move(buf))
	{
		if (_data.storage == nullptr) {
			_data = ByteBuffer(108);
		}
		_data.ResetCurrentStorageCapacitySizeOffset();
		_data.storage->size = 0;
		_data.storage->capacity -= 8;
		_data.storage->offset += 8;
	}

	ByteWriter(ByteWriter &&o)
		: bitscpp::v2::ByteWriter_ByteBuffer(&_data), _data(std::move(o._data))
	{
	}

	inline ByteBuffer &Buffer() { return _data; }

	ByteWriter(uint32_t initialCapacity) : Base(nullptr), _data(initialCapacity + 8)
	{
		_data.storage->size = 0;
		// additional store in form of:
		// [4] - icon7::Flags
		// [4] - max store for header
		_data.storage->capacity -= 8;
		_data.storage->offset += 8;
		Init(&_data);
	}

	void Reinit(uint32_t capacity)
	{
		_data.Init(capacity);
		_data.storage->size = 0;
		_data.storage->capacity -= 8;
		_data.storage->offset += 8;
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
