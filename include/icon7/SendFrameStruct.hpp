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

#ifndef ICON7_SEND_FRAME_STRUCT_HPP
#define ICON7_SEND_FRAME_STRUCT_HPP

#include <cstdint>

#include "Flags.hpp"
#include "ByteBuffer.hpp"

namespace icon7
{
struct SendFrameStruct {
	ByteBuffer data;
	Flags flags = 0;
	uint32_t bytesSent = 0;

	inline uint32_t GetBytes() const { return data.size(); }

	SendFrameStruct(ByteBuffer &dataWithoutHeader);
	SendFrameStruct(ByteBuffer &&dataWithoutHeader);
	inline SendFrameStruct() {}
	inline SendFrameStruct(SendFrameStruct &&o)
		: data(std::move(o.data)), flags(o.flags), bytesSent(o.bytesSent)
	{
		o.data.storage = nullptr;
	}
	inline SendFrameStruct &operator=(SendFrameStruct &&o)
	{
		data = std::move(o.data);
		o.data.storage = nullptr;
		flags = o.flags;
		bytesSent = o.bytesSent;
		return *this;
	}
	inline ~SendFrameStruct()
	{
		data.~ByteBuffer();
		data.storage = nullptr;
	}

	SendFrameStruct(SendFrameStruct &) = delete;
	SendFrameStruct(const SendFrameStruct &) = delete;
	SendFrameStruct &operator=(SendFrameStruct &) = delete;
	SendFrameStruct &operator=(const SendFrameStruct &) = delete;
};
} // namespace icon7

#endif
