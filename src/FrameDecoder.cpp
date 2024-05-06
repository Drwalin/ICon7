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

#include <cstdint>

#include <algorithm>

#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Debug.hpp"

#include "../include/icon7/FrameDecoder.hpp"

namespace icon7
{
FrameDecoder::FrameDecoder() { Restart(); }

void FrameDecoder::PushData(uint8_t *data, uint32_t length,
							void (*onPacket)(ByteBuffer &buffer,
											 uint32_t headerSize,
											 void *userPtr),
							void *userPtr)
{
	while (length) {
		if (headerSize == 0) {
			headerSize = FramingProtocol::GetPacketHeaderSize(data[0]);
		}
		if (buffer.size() < headerSize) {
			uint32_t bytes =
				std::min<uint32_t>(length, headerSize - buffer.size());
			buffer.append(data, bytes);
			length -= bytes;
			data += bytes;
		}
		if (buffer.size() < headerSize) {
			return;
		}
		if (buffer.size() == headerSize) {
			frameSize = headerSize + FramingProtocol::GetPacketBodySize(
										 buffer.data(), headerSize);
			buffer.reserve(frameSize);
		}

		if (buffer.size() < frameSize) {
			uint32_t bytes =
				std::min<uint32_t>(length, frameSize - buffer.size());
			buffer.append(data, bytes);
			length -= bytes;
			data += bytes;
		}

		if (buffer.size() == frameSize) {
			if (onPacket) {
				onPacket(buffer, headerSize, userPtr);
			}
			Restart();
		} else if (buffer.size() > frameSize) {
			LOG_ERROR("FrameDecoder::PushData push to frame more than frame "
					  "size was.");
			throw;
		} else {
			break;
		}
	}
}

void FrameDecoder::Restart()
{
	frameSize = 0;
	headerSize = 0;
	if (buffer.storage && buffer.storage->refCounter.load() == 1) {
		buffer.storage->size = 0;
	} else {
		buffer.Init(256);
	}
}
} // namespace icon7
