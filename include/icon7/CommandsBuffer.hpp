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

#ifndef ICON7_COMMANDS_BUFFER_HPP
#define ICON7_COMMANDS_BUFFER_HPP

#include <cstdint>

#include <utility>

#include "Command.hpp"

namespace icon7
{
class CommandsBuffer
{
public:
	struct CommandEntryMetadata {
		uint32_t offset;
	};

	CommandsBuffer(uint32_t size);
	CommandsBuffer();
	CommandsBuffer(CommandsBuffer &&);
	CommandsBuffer &operator=(CommandsBuffer &&);
	~CommandsBuffer();

	void ExecuteAll();
	void CallDestructors();

	bool CanAllocate(uint32_t size, uint32_t alignement) const;
	template <typename T> bool CanEnqueueCommand() const
	{
		return CanAllocate(sizeof(T), alignof(T));
	}

	template <typename T, typename... Args> bool EnqueueCommand(Args &&...args)
	{
		void *ptr = Allocate(sizeof(T), alignof(T));
		if (ptr == nullptr) {
			return false;
		}
		new (ptr) T(std::move(args)...);
		return true;
	}

	void *Allocate(uint32_t size, uint32_t alignement);

	uint32_t GetMetadataOffset(uint32_t id) const;
	CommandEntryMetadata *GetMetadataPtr(uint32_t id);

	uint32_t GetOffsetOfNextCommand(uint32_t alignement) const;
	template <typename T> uint32_t GetOffsetOfNextCommand() const
	{
		return GetOffsetOfNextCommand(alignof(T));
	}

	Command *GetCommand(uint32_t id);

	inline bool IsValid() const { return commandsBufferData != nullptr; }

	friend class CommandsBufferHandler;
	friend class CommandExecutionQueue;

private:
public:
	uint8_t *commandsBufferData = nullptr;
	uint32_t totalBytes;

	uint32_t countCommands = 0;
	uint32_t commandsExecuted = 0;

	uint32_t offsetOfFree = 0;
};
} // namespace icon7

#endif
