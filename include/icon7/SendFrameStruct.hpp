// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
