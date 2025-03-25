// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

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
