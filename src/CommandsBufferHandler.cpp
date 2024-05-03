/*
 *  This file is part of ICon7.
 *  Copyright (C) 2024 Marek Zalewski aka Drwalin
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
		new (&currentCommandsBuffer) CommandsBuffer();
	}
}

void CommandsBufferHandler::AllocateBuffer()
{
	FlushBuffer();
	currentCommandsBuffer.~CommandsBuffer();
	new (&currentCommandsBuffer) CommandsBuffer(bufferSize);
}
} // namespace icon7
