/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
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

#include <chrono>

#include "../concurrentqueue/concurrentqueue.h"

#include "../include/icon7/Debug.hpp"

#include "../include/icon7/CommandExecutionQueue.hpp"

namespace icon7
{
using QueueType = moodycamel::ConcurrentQueue<CommandHandle<Command>>;

CommandExecutionQueue::CommandExecutionQueue()
{
	asyncExecutionFlags = STOPPED;
	concurrentQueueCommands = new QueueType();
}

CommandExecutionQueue::~CommandExecutionQueue()
{
	WaitStopAsyncExecution();
	delete (QueueType *)concurrentQueueCommands;
	concurrentQueueCommands = nullptr;
}

void CommandExecutionQueue::EnqueueCommand(CommandHandle<Command> &&command)
{
	if (command.IsValid() == false) {
		LOG_ERROR("Trying to enqueue empty command");
	} else {
		((QueueType *)concurrentQueueCommands)->enqueue(std::move(command));
	}
}

uint32_t
CommandExecutionQueue::TryDequeueBulkAny(CommandHandle<Command> *commands,
										 uint32_t max)
{
	size_t size = ((QueueType *)concurrentQueueCommands)->size_approx();
	if (size == 0)
		size = 1;
	size = std::min<size_t>(size, max);
	size_t dequeued = ((QueueType *)concurrentQueueCommands)
						  ->try_dequeue_bulk(commands, size);
	return dequeued;
}

void CommandExecutionQueue::QueueStopAsyncExecution()
{
	asyncExecutionFlags |= QUEUE_STOP;
}

void CommandExecutionQueue::WaitStopAsyncExecution()
{
	QueueStopAsyncExecution();
	while (IsRunningAsync()) {
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}
}

bool CommandExecutionQueue::IsRunningAsync() const
{
	return asyncExecutionFlags.load() & IS_RUNNING;
}

void CommandExecutionQueue::RunAsyncExecution(
	uint32_t sleepMicrosecondsOnNoActions)
{
	std::thread(&CommandExecutionQueue::ExecuteLoop, this,
				sleepMicrosecondsOnNoActions)
		.detach();
}

void CommandExecutionQueue::ExecuteLoop(uint32_t sleepMicrosecondsOnNoActions)
{
	asyncExecutionFlags = IS_RUNNING;
	while (asyncExecutionFlags.load() == IS_RUNNING) {
		uint32_t dequeued = Execute(128);
		if (dequeued == 0) {
			std::this_thread::sleep_for(
				std::chrono::microseconds(sleepMicrosecondsOnNoActions));
		}
	}
	asyncExecutionFlags = STOPPED;
}

uint32_t CommandExecutionQueue::Execute(uint32_t maxToDequeue)
{
	const uint32_t MAX_DEQUEUE_COMMANDS = 128;
	CommandHandle<Command> commands[MAX_DEQUEUE_COMMANDS];
	const uint32_t dequeued = TryDequeueBulkAny(
		commands, maxToDequeue < MAX_DEQUEUE_COMMANDS ? maxToDequeue
													  : MAX_DEQUEUE_COMMANDS);
	if (dequeued > 0) {
		for (uint32_t i = 0; i < dequeued; ++i) {
			commands[i].Execute();
		}
	}
	return dequeued;
}

bool CommandExecutionQueue::HasAny() const
{
	return ((QueueType *)concurrentQueueCommands)->size_approx() != 0;
}
} // namespace icon7
