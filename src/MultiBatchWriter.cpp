// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"

#include "../include/icon7/MultiBatchWriter.hpp"

namespace icon7
{
SendBatch::~SendBatch()
{
	Flush();
}

void SendBatch::Push(PeerHandle handle, ByteBufferReadable &&frame)
{
	if (loop == nullptr) {
		[[unlikely]];
		loop = handle.loop;
	}
	assert(loop == handle.loop);
	if (data.capacity() < MAX_BUFFERERED_FRAMES) {
		[[unlikely]];
		data.reserve(MAX_BUFFERERED_FRAMES);
	}
	data.emplace_back(std::move(frame), handle.id);
}

void SendBatch::Flush()
{
	if (data.empty() == false && loop == nullptr) {
		LOG_FATAL("Non empty buffer batch sends with empty loop");
		data.clear();
		return;
	}
	if (data.empty()) {
		return;
	}
	
	class CommandBatchSend final : public commands::ExecuteOnLoop
	{
	public:
		CommandBatchSend() {}
		virtual ~CommandBatchSend() {}
		virtual void Execute() override {
			for (SendData &d : data) {
				PeerHandle handle = {d.peerId, loop};
				PeerData *pd = loop->GetLocalPeerData(handle);
				if (pd) {
					pd->Send(std::move(d.frame));
				}
			}
		}
		std::vector<SendData> data;
	};
	auto com = CommandHandle<CommandBatchSend>::Create();
	com->loop = loop;
	com->data = std::move(data);
	loop->EnqueueCommand(std::move(com));
}

MultiBatchWriter::~MultiBatchWriter()
{
	Flush();
}

void MultiBatchWriter::Send(PeerHandle handle, ByteBufferReadable &frame)
{
	ByteBufferReadable _frame = frame;
	Send(handle, std::move(_frame));
}

void MultiBatchWriter::Send(PeerHandle handle, ByteBufferReadable &&frame)
{
	SendBatch &batch = map[handle.loop];
	batch.Push(handle, std::move(frame));
}

void MultiBatchWriter::Flush()
{
	for (int i=0; i<map.data.size(); ++i) {
		SendBatch &sb = map.data[i];
		if (sb.loop == nullptr) {
			continue;
		}
		sb.Flush();
	}
}
} // namespace icon7
