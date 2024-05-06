/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
 *
 *  ICon7 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ICON7_BYTE_WRITER_HPP
#define ICON7_BYTE_WRITER_HPP

#include "../../bitscpp/include/bitscpp/ByteWriter.hpp"

#include "ByteBuffer.hpp"

namespace icon7
{
class ByteWriter : public bitscpp::ByteWriter<ByteBuffer>
{
public:
	ByteBuffer _data;
	using Base = bitscpp::ByteWriter<ByteBuffer>;

public:
	~ByteWriter(){};

	ByteWriter(ByteBuffer &&buf) : _data(std::move(buf))
	{
		if (_data.storage == nullptr) {
			_data = ByteBuffer(108);
		}
		_data.ResetCurrentStorageCapacitySizeOffset();
		_data.storage->size = 0;
		_data.storage->capacity -= 8;
		_data.storage->offset += 8;
		Init(&_data);
	}

	ByteWriter(ByteWriter &&o)
		: bitscpp::ByteWriter<ByteBuffer>(&_data), _data(std::move(o._data))
	{
	}

	ByteWriter(uint32_t initialCapacity) : _data(initialCapacity + 8)
	{
		_data.storage->size = 0;
		// additional store in form of:
		// [4] - icon7::Flags
		// [4] - max store for header
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
