/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
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

#ifndef ICON7_BYTE_READER_HPP
#define ICON7_BYTE_READER_HPP

#include "../../bitscpp/include/bitscpp/ByteReader.hpp"

#include "ByteBuffer.hpp"

namespace icon7
{
class ByteReader : public bitscpp::ByteReader<true>
{
public:
	ByteBuffer _data;

public:
	inline ByteReader() : bitscpp::ByteReader<true>(nullptr, 0, 0) {}

	inline ByteReader(ByteBuffer &data, uint32_t offset)
		: bitscpp::ByteReader<true>(data.data(), offset, data.size()),
		  _data(data)
	{
	}

	inline ByteReader(ByteBuffer &&data, uint32_t offset)
		: bitscpp::ByteReader<true>(data.data(), offset, data.size()),
		  _data(std::move(data))
	{
	}

	inline ~ByteReader() {}

	inline ByteReader(ByteReader &&o)
		: bitscpp::ByteReader<true>(o._buffer, o.get_offset(), o._size),
		  _data(std::move(o._data))
	{
		errorReading_bufferToSmall = o.errorReading_bufferToSmall;
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
