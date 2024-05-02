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

#ifndef ICON7_SEND_FRAME_STRUCT_HPP
#define ICON7_SEND_FRAME_STRUCT_HPP

#include <cstdint>

#include <vector>

#include "Flags.hpp"

namespace icon7
{
struct SendFrameStruct {
	std::vector<uint8_t> dataWithoutHeader;
	uint32_t bytesSent = 0;
	Flags flags = 0;
	uint8_t header[4] = {0, 0, 0, 0};
	uint8_t headerBytesSent = 0;
	uint8_t headerSize = 0;

	inline uint32_t GetBytes() const
	{
		return headerSize + dataWithoutHeader.size();
	}

	static SendFrameStruct Acquire(std::vector<uint8_t> &&dataWithoutHeader,
								   Flags flags);
	static void Release(SendFrameStruct &ptr);

	SendFrameStruct(std::vector<uint8_t> &&dataWithoutHeader, Flags flags);
	SendFrameStruct();
	~SendFrameStruct();

	// TODO: mark copy semantics as deleted

	SendFrameStruct(SendFrameStruct &) = delete;
	SendFrameStruct(const SendFrameStruct &) = delete;
	SendFrameStruct(SendFrameStruct &&) = default;

	SendFrameStruct &operator=(SendFrameStruct &) = delete;
	SendFrameStruct &operator=(const SendFrameStruct &) = delete;
	SendFrameStruct &operator=(SendFrameStruct &&) = default;
};
} // namespace icon7

#endif
