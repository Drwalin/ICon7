// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <memory>
#include <thread>
#include <utility>

#include "../concurrentqueue/concurrentqueue.h"

#include "../include/icon7/Time.hpp"
#include "../include/icon7/ConcurrentQueueTraits.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/CommandsBufferHandler.hpp"
#include "../include/icon7/CoroutineHelper.hpp"
#include "../include/icon7/Debug.hpp"

#include "../include/icon7/CommandExecutionQueue.hpp"

namespace icon7
{
void *CoroutineSchedulable::promise_type::operator new(std::size_t bytes)
{
	return MemoryPool::Allocate(bytes).object;
}

void CoroutineSchedulable::promise_type::operator delete(void *ptr,
														 std::size_t bytes)
{
	MemoryPool::Release(ptr, bytes);
}

void CommandExecutionQueue::CoroutineAwaitable::await_suspend(
	std::coroutine_handle<> h)
{
	queue->EnqueueCommand(CommandHandle<>::Create(h));
	queue = nullptr;
	objectHolder = nullptr;
}

CommandExecutionQueue::CommandExecutionQueue()
{
	asyncExecutionFlags = STOPPED;
	queue = new moodycamel::ConcurrentQueue<CommandHandle<Command>,
											ConcurrentQueueDefaultTraits>();
}

CommandExecutionQueue::~CommandExecutionQueue()
{
	WaitStopAsyncExecution();
	delete queue;
	queue = nullptr;
}

void CommandExecutionQueue::EnqueueCommand(CommandHandle<Command> &&command)
{
	if (command.IsValid() == false) {
		LOG_ERROR("Trying to enqueue empty command");
	} else {
		queue->enqueue(std::move(command));
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
	if (queue->try_dequeue(command)) {
		return true;
	} else {
		command._com = nullptr;
		return false;
	}
}

size_t CommandExecutionQueue::TryDequeueBulk(CommandHandle<Command> *commands,
											 size_t max)
{
	return queue->try_dequeue_bulk(commands, max);
}

void CommandExecutionQueue::WaitStopAsyncExecution()
{
	QueueStopAsyncExecution();
	while (IsRunningAsync()) {
		icon7::time::Sleep(icon7::time::microseconds(10));
	}
}

bool CommandExecutionQueue::IsRunningAsync() const
{
	return asyncExecutionFlags.load() & IS_RUNNING;
}

void CommandExecutionQueue::RunAsyncExecution(
	uint32_t sleepMicrosecondsOnNoActions, uint32_t maxSleepDuration)
{
	std::thread(&CommandExecutionQueue::ExecuteLoop, this,
				sleepMicrosecondsOnNoActions, maxSleepDuration)
		.detach();
}

void CommandExecutionQueue::ExecuteLoop(uint32_t sleepMicrosecondsOnNoActions,
										uint32_t maxSleepDuration)
{
	asyncExecutionFlags = IS_RUNNING;
	uint32_t accumulativeNopCounter = 0;
	while (asyncExecutionFlags.load() == IS_RUNNING) {
		uint32_t dequeued = Execute(128);
		if (dequeued == 0) {
			uint64_t sleepTime =
				sleepMicrosecondsOnNoActions + accumulativeNopCounter * 64;
			accumulativeNopCounter++;
			if (sleepTime < 1)
				sleepTime = 1;
			if (sleepTime > maxSleepDuration)
				sleepTime = maxSleepDuration;
			icon7::time::Sleep(icon7::time::microseconds(sleepTime));
		} else {
			accumulativeNopCounter = 0;
		}
	}
	asyncExecutionFlags = STOPPED;
}

uint32_t CommandExecutionQueue::Execute(uint32_t maxToDequeue)
{
	constexpr uint32_t elems = 128;
	uint32_t total = 0;
	alignas(alignof(CommandHandle<Command>))
		uint8_t commandsStore[elems * sizeof(CommandHandle<Command>)];
	memset(commandsStore, 0, elems * sizeof(CommandHandle<Command>));
	CommandHandle<Command> *commands = (CommandHandle<Command> *)commandsStore;

	while (maxToDequeue) {
		uint32_t toDequeue = std::min<uint32_t>(maxToDequeue, elems);
		uint32_t dequeued = TryDequeueBulk(commands, toDequeue);

		total += dequeued;
		for (int i = 0; i < dequeued; ++i) {
			commands[i].Execute();
			commands[i].~CommandHandle();
			commands[i]._com = nullptr;
		}

		if (dequeued != toDequeue) {
			break;
		} else {
			maxToDequeue -= dequeued;
		}
	}
	return total;
}

bool CommandExecutionQueue::HasAny() const { return queue->size_approx() != 0; }

} // namespace icon7
