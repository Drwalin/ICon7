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
#include <memory>
#include <thread>
#include <utility>
#include <unordered_map>

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/CommandExecutionQueue.hpp"

namespace icon7
{
CommandExecutionQueue::CommandExecutionQueue() : queue()
{
	asyncExecutionFlags = STOPPED;
}

CommandExecutionQueue::~CommandExecutionQueue() { WaitStopAsyncExecution(); }

void CommandExecutionQueue::EnqueueCommand(CommandHandle<Command> &&command)
{
	if (command.IsValid() == false) {
		LOG_ERROR("Trying to enqueue empty command");
	} else {
		queue.enqueue(std::move(command));
		command._com = nullptr;
	}
}

void CommandExecutionQueue::EnqueueCommandsBuffer(CommandsBuffer &&buffer)
{
	class CommandExecuteCommandsBuffer final : public Command
	{
	public:
		CommandExecuteCommandsBuffer() {}
		virtual ~CommandExecuteCommandsBuffer() {}
		CommandsBuffer buffer;
		virtual void Execute() override { buffer.ExecuteAll(); }
	};
	auto com = CommandHandle<CommandExecuteCommandsBuffer>::Create();
	com->buffer = std::move(buffer);
	EnqueueCommand(std::move(com));
}

void CommandExecutionQueue::QueueStopAsyncExecution()
{
	asyncExecutionFlags |= QUEUE_STOP;
}

bool CommandExecutionQueue::TryDequeue(CommandHandle<Command> &command)
{
	command.~CommandHandle();
	if (queue.try_dequeue(command)) {
		return true;
	} else {
		command._com = nullptr;
		return false;
	}
}

size_t CommandExecutionQueue::TryDequeueBulk(CommandHandle<Command> *commands,
											 size_t max)
{
	return queue.try_dequeue_bulk(commands, max);
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
	uint32_t total = 0;
	CommandHandle<Command> commands[128];
	while (maxToDequeue && HasAny()) {
		uint32_t toDequeue = std::min<uint32_t>(maxToDequeue, 128);
		uint32_t dequeued = TryDequeueBulk(commands, toDequeue);
		maxToDequeue -= dequeued;
		total += dequeued;
		for (int i = 0; i < dequeued; ++i) {
			commands[i].Execute();
			commands[i].~CommandHandle();
		}
	}
	return total;
}

bool CommandExecutionQueue::HasAny() const { return queue.size_approx() != 0; }

CommandsBufferHandler *CommandExecutionQueue::GetThreadLocalBuffer()
{
	thread_local std::unordered_map<CommandExecutionQueue *,
									std::unique_ptr<CommandsBufferHandler>>
		buffers;
	auto &b = buffers[this];
	if (b.get() == nullptr) {
		b = CreateCommandBufferHandler();
	}
	return b.get();
}

std::unique_ptr<CommandsBufferHandler>
CommandExecutionQueue::CreateCommandBufferHandler()
{
	return std::make_unique<CommandsBufferHandler>(this);
}

void CommandExecutionQueue::FlushThreadLocalCommandsBuffer()
{
	GetThreadLocalBuffer()->FlushBuffer();
}

} // namespace icon7
