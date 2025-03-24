// Copyright (C) 2024-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_COMMANDS_BUFFER_HANDLER_HPP
#define ICON7_COMMANDS_BUFFER_HANDLER_HPP

#include <chrono>

#include "Debug.hpp"
#include "CommandsBuffer.hpp"

namespace icon7
{
class CommandExecutionQueue;

class CommandsBufferHandler
{
public:
	CommandsBufferHandler(CommandExecutionQueue *queue);
	~CommandsBufferHandler();

	void FlushBuffer();
	void AllocateBuffer();

	template <typename T, typename... Args> void EnqueueCommand(Args &&...args)
	{
		if (currentCommandsBuffer.IsValid() == false ||
			currentCommandsBuffer.CanAllocate(sizeof(T), alignof(T)) == false) {
			AllocateBuffer();
		}
		if (forceEnqueueTimepoint < std::chrono::steady_clock::now()) {
			AllocateBuffer();
		}
		if (currentCommandsBuffer.countCommands == 0) {
			forceEnqueueTimepoint = std::chrono::steady_clock::now() +
									std::chrono::microseconds(200000);
		}
		if (currentCommandsBuffer.EnqueueCommand<T>(std::move(args)...) ==
			false) {
			LOG_FATAL("Failed to enqueue command");
		}
	}

	template <typename T, typename... Args>
	void EnqueueCommandAndFlush(Args &&...args)
	{
		EnqueueCommand<T>(std::move(args)...);
		FlushBuffer();
	}

private:
public:
	inline const static uint32_t bufferSize = 1024 * 16;
	CommandExecutionQueue *queue;
	CommandsBuffer currentCommandsBuffer;
	std::chrono::steady_clock::time_point forceEnqueueTimepoint;
};
} // namespace icon7

#endif
