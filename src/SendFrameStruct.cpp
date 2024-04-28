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

#include <cstring>

#include "../include/icon7/FramingProtocol.hpp"

#include "../include/icon7/SendFrameStruct.hpp"

namespace icon7
{

SendFrameStruct::SendFrameStruct(std::vector<uint8_t> &&_dataWithoutHeader,
								 Flags flags)
	: dataWithoutHeader(std::move(_dataWithoutHeader))
{
	this->flags = flags;
	bytesSent = 0;
	headerBytesSent = 0;
	headerSize = FramingProtocol::GetHeaderSize(dataWithoutHeader.size());
	*(uint32_t *)header = 0;
	FramingProtocol::WriteHeader(header, headerSize, dataWithoutHeader.size(),
								 flags);
}
SendFrameStruct::SendFrameStruct() {}

SendFrameStruct::SendFrameStruct(SendFrameStruct &o)
{
	dataWithoutHeader = o.dataWithoutHeader;
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
}

SendFrameStruct::SendFrameStruct(const SendFrameStruct &o)
{
	dataWithoutHeader = o.dataWithoutHeader;
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
}

SendFrameStruct::SendFrameStruct(SendFrameStruct &&o)
{
	dataWithoutHeader = std::move(o.dataWithoutHeader);
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
	o.__m_next = nullptr;
}

SendFrameStruct &SendFrameStruct::operator=(SendFrameStruct &o)
{
	dataWithoutHeader = o.dataWithoutHeader;
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
	return *this;
}

SendFrameStruct &SendFrameStruct::operator=(const SendFrameStruct &o)
{
	dataWithoutHeader = o.dataWithoutHeader;
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
	return *this;
}

SendFrameStruct &SendFrameStruct::operator=(SendFrameStruct &&o)
{
	dataWithoutHeader = std::move(o.dataWithoutHeader);
	memcpy(header, o.header, 4);
	headerBytesSent = o.headerBytesSent;
	headerSize = o.headerSize;
	__m_next.store(o.__m_next.load());
	o.__m_next = nullptr;
	return *this;
}

} // namespace icon7
