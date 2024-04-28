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
#include <thread>

#include "../include/icon7/Debug.hpp"

#include "../include/icon7/CommandExecutionQueue.hpp"

namespace icon7
{
CommandExecutionQueue::CommandExecutionQueue()
{
	asyncExecutionFlags = STOPPED;
}

CommandExecutionQueue::~CommandExecutionQueue() { WaitStopAsyncExecution(); }

void CommandExecutionQueue::EnqueueCommand(CommandHandle<Command> &&command)
{
	if (command.IsValid() == false) {
		LOG_ERROR("Trying to enqueue empty command");
	} else {
		queue.push(command._com);
		command._com = nullptr;
	}
}

void CommandExecutionQueue::QueueStopAsyncExecution()
{
	asyncExecutionFlags |= QUEUE_STOP;
}

bool CommandExecutionQueue::TryDequeue(CommandHandle<Command> &command)
{
	command.~CommandHandle();
	Command *ptr = queue.pop();
	if (ptr) {
		command._com = ptr;
		return true;
	} else {
		command._com = nullptr;
		return false;
	}
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
	uint32_t i = 0;
	CommandHandle<Command> com;
	for (; i < maxToDequeue; ++i) {
		if (TryDequeue(com)) {
			com.Execute();
		} else {
			break;
		}
	}
	return i;
}

bool CommandExecutionQueue::HasAny() const { return false; }
} // namespace icon7
