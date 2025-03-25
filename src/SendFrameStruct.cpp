// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/SendFrameStruct.hpp"

namespace icon7
{
SendFrameStruct::SendFrameStruct(ByteBuffer &data) : data(data)
{
	memcpy(&flags,
		   ((uint8_t *)this->data.storage) + sizeof(ByteBufferStorageHeader),
		   4);
	bytesSent = 0;
}
SendFrameStruct::SendFrameStruct(ByteBuffer &&data) : data(std::move(data))
{
	memcpy(&flags,
		   ((uint8_t *)this->data.storage) + sizeof(ByteBufferStorageHeader),
		   4);
	bytesSent = 0;
}
} // namespace icon7
