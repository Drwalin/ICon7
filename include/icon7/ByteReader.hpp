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

#ifndef ICON7_BYTE_READER_HPP
#define ICON7_BYTE_READER_HPP

#include "../../bitscpp/include/bitscpp/ByteReader.hpp"

namespace icon7
{
class ByteReader : public bitscpp::ByteReader<true>
{
public:
	std::vector<uint8_t> _data;

public:
	ByteReader(std::vector<uint8_t> &data, uint32_t offset)
		: bitscpp::ByteReader<true>(nullptr, offset, data.size()), _data(data)
	{
		buffer = _data.data();
	}

	ByteReader(std::vector<uint8_t> &&data, uint32_t offset)
		: bitscpp::ByteReader<true>(data.data(), offset, data.size()),
		  _data(std::move(data))
	{
	}

	~ByteReader() = default;

	ByteReader(ByteReader &&o)
		: bitscpp::ByteReader<true>(o.buffer, o.offset, o.size),
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

	inline uint8_t const *data() { return buffer; }

	ByteReader(ByteReader &) = delete;
	ByteReader(const ByteReader &) = delete;
	ByteReader &operator=(ByteReader &) = delete;
	ByteReader &operator=(const ByteReader &) = delete;
};
} // namespace icon7

#endif
