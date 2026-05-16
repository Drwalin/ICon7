// Copyright (C) 2024-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/CommandExecutionQueue.hpp"
#include "../include/icon7/CommandsBufferHandler.hpp"

namespace icon7
{
CommandsBufferHandler::CommandsBufferHandler(CommandExecutionQueue *queue)
{
	this->queue = queue;
	forceEnqueueTimepoint = time::GetTemporaryTimestamp() + time::milliseconds(200);
}

CommandsBufferHandler::~CommandsBufferHandler()
{
	FlushBuffer();
	queue = nullptr;
	currentCommandsBuffer.Reset();
}

void CommandsBufferHandler::FlushBuffer()
{
	if (queue == nullptr) {
		LOG_FATAL("Queue is empty.");
		return;
	}
	if (currentCommandsBuffer.IsValid() &&
		currentCommandsBuffer.countCommands != 0) {
		queue->EnqueueCommandsBuffer(std::move(currentCommandsBuffer));
		currentCommandsBuffer.Reset();
	}
}

void CommandsBufferHandler::AllocateBuffer()
{
	FlushBuffer();
	assert(currentCommandsBuffer.countCommands == 0);
	if (!currentCommandsBuffer.IsValid()) {
		currentCommandsBuffer.Reset();
		currentCommandsBuffer.Init(bufferSize);
	}
	assert(currentCommandsBuffer.countCommands == 0);
}
} // namespace icon7
