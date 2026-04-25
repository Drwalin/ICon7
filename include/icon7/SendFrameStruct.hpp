// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_SEND_FRAME_STRUCT_HPP
#define ICON7_SEND_FRAME_STRUCT_HPP

#include <cstdint>

#include "ByteBuffer.hpp"

namespace icon7
{
struct SendFrameStruct {
	ByteBufferReadable data;
	uint32_t bytesSent = 0;

	inline uint32_t GetBytes() const { return data.size(); }

	SendFrameStruct(ByteBufferReadable &dataWithoutHeader);
	SendFrameStruct(ByteBufferReadable &&dataWithoutHeader);
	inline SendFrameStruct() {}
	inline SendFrameStruct(SendFrameStruct &&o)
		: data(std::move(o.data)), bytesSent(o.bytesSent)
	{
		o.data.storage = nullptr;
	}
	inline SendFrameStruct &operator=(SendFrameStruct &&o)
	{
		data = std::move(o.data);
		o.data.storage = nullptr;
		bytesSent = o.bytesSent;
		return *this;
	}
	inline ~SendFrameStruct()
	{
	}

	SendFrameStruct(SendFrameStruct &) = delete;
	SendFrameStruct(const SendFrameStruct &) = delete;
	SendFrameStruct &operator=(SendFrameStruct &) = delete;
	SendFrameStruct &operator=(const SendFrameStruct &) = delete;
};
} // namespace icon7

#endif
