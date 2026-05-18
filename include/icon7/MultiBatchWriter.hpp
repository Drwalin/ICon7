// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_MULTI_BATCH_WRITER_HPP
#define ICON7_MULTI_BATCH_WRITER_HPP

#include <vector>

#include "PeerHandle.hpp"
#include "MapLoop.hpp"

namespace icon7
{
struct SendBatch
{
	inline const static uint32_t MAX_BUFFERERED_FRAMES = 256;
	struct SendData {
		ByteBufferReadable frame;
		PeerId peerId;
	};

	Loop *loop = nullptr;
	std::vector<SendData> data;

	~SendBatch();

	void Push(PeerHandle handle, ByteBufferReadable &&frame);
	void Flush();
};

class MultiBatchWriter
{
public:
	~MultiBatchWriter();

	void Send(PeerHandle handle, ByteBufferReadable &frame);
	void Send(PeerHandle handle, ByteBufferReadable &&frame);

	void Flush();

private:
	MapLoop<SendBatch> map;
};
} // namespace icon7

#endif
