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

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/MemoryPool.hpp"

#include "../include/icon7/CommandsBuffer.hpp"

namespace icon7
{
CommandsBuffer::CommandsBuffer()
{
	commandsBufferData = nullptr;
	totalBytes = 0;
	countCommands = 0;
	commandsExecuted = 0;
	offsetOfFree = 0;
}

CommandsBuffer::CommandsBuffer(uint32_t size)
{
	if ((size & 0x3F) != 0) {
		LOG_FATAL("CommandsBuffer needs to have size of multiple of 64");
	}
	// TODO: use pool allocator
	totalBytes = size;
	commandsBufferData = (uint8_t *)MemoryPool::Allocate(totalBytes);
	countCommands = 0;
	commandsExecuted = 0;
	offsetOfFree = 0;
}

CommandsBuffer::CommandsBuffer(CommandsBuffer &&o)
{
	commandsBufferData = o.commandsBufferData;
	totalBytes = o.totalBytes;
	countCommands = o.countCommands;
	commandsExecuted = o.commandsExecuted;
	offsetOfFree = o.offsetOfFree;

	o.commandsBufferData = nullptr;
	o.totalBytes = 0;
	o.countCommands = 0;
	o.commandsExecuted = 0;
	o.offsetOfFree = 0;
}

CommandsBuffer &CommandsBuffer::operator=(CommandsBuffer &&o)
{
	this->~CommandsBuffer();
	commandsBufferData = o.commandsBufferData;
	totalBytes = o.totalBytes;
	countCommands = o.countCommands;
	commandsExecuted = o.commandsExecuted;
	offsetOfFree = o.offsetOfFree;
	return *this;
}

CommandsBuffer::~CommandsBuffer()
{
	if (commandsBufferData) {
		CallDestructors();
		MemoryPool::Release(commandsBufferData, totalBytes);
		commandsBufferData = nullptr;
	}
}

void CommandsBuffer::ExecuteAll()
{
	for (; commandsExecuted < countCommands; ++commandsExecuted) {
		Command *com = GetCommand(commandsExecuted);
		com->Execute();
		com->~Command();
	}
}

void CommandsBuffer::CallDestructors()
{
	for (; commandsExecuted < countCommands; ++commandsExecuted) {
		Command *com = GetCommand(commandsExecuted);
		com->~Command();
	}
}

bool CommandsBuffer::CanAllocate(uint32_t size, uint32_t alignement) const
{
	const uint32_t commandId = countCommands;
	const uint32_t metaOffset = GetMetadataOffset(commandId);
	const uint32_t commandOffset = GetOffsetOfNextCommand(alignement);
	if (commandOffset + size > metaOffset) {
		return false;
	}
	return true;
}

void *CommandsBuffer::Allocate(uint32_t size, uint32_t alignement)
{
	if (CanAllocate(size, alignement) == false) {
		return nullptr;
	}
	const uint32_t commandId = countCommands;
	const uint32_t commandOffset = GetOffsetOfNextCommand(alignement);
	++countCommands;
	GetMetadataPtr(commandId)->offset = commandOffset;
	offsetOfFree = commandOffset + size;
	return commandsBufferData + commandOffset;
}

uint32_t CommandsBuffer::GetMetadataOffset(uint32_t id) const
{
	return totalBytes - sizeof(CommandEntryMetadata) * (id + 1);
}

CommandsBuffer::CommandEntryMetadata *
CommandsBuffer::GetMetadataPtr(uint32_t id)
{
	return (CommandEntryMetadata *)(commandsBufferData + GetMetadataOffset(id));
}

uint32_t CommandsBuffer::GetOffsetOfNextCommand(uint32_t alignement) const
{
	uint32_t offset = offsetOfFree + alignement - 1;
	offset &= ~(alignement - 1);
	return offset;
}

Command *CommandsBuffer::GetCommand(uint32_t id)
{
	return (Command *)(commandsBufferData + GetMetadataPtr(id)->offset);
}
} // namespace icon7
