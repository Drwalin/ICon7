// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdint>

#include <algorithm>

#include "../include/icon7/FramingProtocol.hpp"
#include "../include/icon7/Debug.hpp"

#include "../include/icon7/FrameDecoder.hpp"

namespace icon7
{
FrameDecoder::FrameDecoder() { Restart(); }

void FrameDecoder::PushData(uint8_t *data, uint32_t _length,
							void (*onPacket)(ByteBuffer &buffer,
											 uint32_t headerSize,
											 void *userPtr),
							void *userPtr)
{
	int length = _length;
	while (length > 0) {
		if (headerSize == 0) {
			headerSize = FramingProtocol::GetPacketHeaderSize(data[0]);
			buffer.append(data, 1);
			length--;
			data++;
		}
		if (buffer.size() < headerSize) {
			uint32_t bytes =
				std::min<uint32_t>(length, headerSize - buffer.size());
			buffer.append(data, bytes);
			length -= bytes;
			data += bytes;
		}
		if (buffer.size() < headerSize) {
			if (length != 0) {
				LOG_FATAL("This error should never happen, FrameDecoder "
						  "algorithm broken: buffer.size(): %u/%u    "
						  "headerSize:   %u    length: %u",
						  frameSize, buffer.size(), headerSize, length);
			}
			break;
		}
		if (buffer.size() == headerSize) {
			frameSize = headerSize + FramingProtocol::GetPacketBodySize(
										 buffer.data(), headerSize);
			buffer.reserve(frameSize);
		} else if (frameSize == 0) {
			LOG_FATAL("This error should never happen - FrameDecoder algorithm "
					  "broken:   length: %u    buffer.size: %u     frameSize: "
					  "%u     headerSize: %u",
					  length, buffer.size(), frameSize, headerSize);
		}

		if (buffer.size() < frameSize) {
			uint32_t bytes = frameSize - buffer.size();
			if (bytes > length) {
				bytes = length;
			}
			if (bytes < 0) {
				LOG_FATAL("This error should never happen - FrameDecoder "
						  "algorithm broken:   length: %u    buffer.size: %u   "
						  "  frameSize: %u     headerSize: %u ;    bytes < 0",
						  length, buffer.size(), frameSize, headerSize, bytes);
			}
			buffer.append(data, bytes);
			length -= bytes;
			data += bytes;
		} else {
			LOG_FATAL("This error should never happen - FrameDecoder algorithm "
					  "broken:   length: %u    buffer.size: %u     frameSize: "
					  "%u     headerSize: %u",
					  length, buffer.size(), frameSize, headerSize);
		}

		if (buffer.size() == frameSize) {
			if (onPacket) {
				onPacket(buffer, headerSize, userPtr);
			}
			Restart();
		} else if (buffer.size() > frameSize) {
			LOG_FATAL("FrameDecoder::PushData push to frame more than frame "
					  "size was: %i > %i, h:%i: 0x%2.2X",
					  buffer.size(), frameSize, headerSize,
					  (uint32_t)(uint8_t)buffer.data()[0]);
			throw;
		} else {
			break;
		}
	}
	if (length < 0) {
		LOG_FATAL("This error should never happen - FrameDecoder algorithm "
				  "broken:   length: %u    buffer.size: %u     frameSize: %u   "
				  "  headerSize: %u",
				  length, buffer.size(), frameSize, headerSize);
	}
}

void FrameDecoder::Restart()
{
	frameSize = 0;
	headerSize = 0;
	buffer.Init(2048);
	buffer.clear();
}
} // namespace icon7
