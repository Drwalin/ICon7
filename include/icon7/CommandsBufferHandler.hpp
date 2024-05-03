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
