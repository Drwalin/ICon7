// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
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
}

CommandsBufferHandler::~CommandsBufferHandler() { FlushBuffer(); }

void CommandsBufferHandler::FlushBuffer()
{
	if (currentCommandsBuffer.IsValid() &&
		currentCommandsBuffer.countCommands != 0) {
		queue->EnqueueCommandsBuffer(std::move(currentCommandsBuffer));
		currentCommandsBuffer.~CommandsBuffer();
		new (&currentCommandsBuffer) CommandsBuffer();
	}
}

void CommandsBufferHandler::AllocateBuffer()
{
	FlushBuffer();
	assert(currentCommandsBuffer.countCommands == 0);
	if (!currentCommandsBuffer.IsValid()) {
		currentCommandsBuffer.~CommandsBuffer();
		new (&currentCommandsBuffer) CommandsBuffer(bufferSize);
	}
	assert(currentCommandsBuffer.countCommands == 0);
}
} // namespace icon7
