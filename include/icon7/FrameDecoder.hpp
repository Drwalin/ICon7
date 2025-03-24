// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FRAME_DECODER_HPP
#define ICON7_FRAME_DECODER_HPP

#include <cstdint>

#include "ByteBuffer.hpp"

namespace icon7
{
class Peer;

class FrameDecoder
{
public:
	FrameDecoder();

	void Restart();
	void PushData(uint8_t *data, uint32_t length,
				  void (*onPacket)(ByteBuffer &buffer, uint32_t headerSize,
								   void *userPtr),
				  void *userPtr);

private:
	ByteBuffer buffer;
	uint32_t headerSize;
	uint32_t frameSize;
};
} // namespace icon7

#endif
