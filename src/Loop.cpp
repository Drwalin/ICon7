// Copyright (C) 2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <chrono>
#include <thread>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Host.hpp"

#include "../include/icon7/Loop.hpp"

namespace icon7
{
Loop::Loop()
{
	userData = 0;
	userPointer = nullptr;
	asyncRunnerFlags = 0;
}

Loop::~Loop() { WaitStopRunning(); }

void Loop::WaitStopRunning()
{
	QueueStopRunning();
	while (IsRunningAsync()) {
		QueueStopRunning();
		WakeUp();
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

void Loop::Destroy()
{
	class CommandDestroyHosts final : public Command
	{
	public:
		CommandDestroyHosts() {}
		virtual ~CommandDestroyHosts() {}
		Loop *loop;
		virtual void Execute() override
		{
			loop->commandQueue.Execute(128);
			while (loop->commandQueue.HasAny()) {
				loop->commandQueue.Execute(128);
			}
			while (loop->hosts.empty() == false) {
				std::shared_ptr<icon7::Host> h = *loop->hosts.begin();
				h->_InternalDestroy();
			}
		}
	};
	auto com = CommandHandle<CommandDestroyHosts>::Create();
	com->loop = this;
	EnqueueCommand(std::move(com));

	WaitStopRunning();

	commandQueue.Execute(128);
	while (commandQueue.HasAny()) {
		commandQueue.Execute(128);
	}
}

void Loop::EnqueueCommand(CommandHandle<Command> &&command)
{
	commandQueue.EnqueueCommand(std::move(command));
	WakeUp();
}

CommandExecutionQueue::CoroutineAwaitable
Loop::Schedule(std::shared_ptr<void> &&obj)
{
	return commandQueue.Schedule(std::move(obj));
}

CommandExecutionQueue::CoroutineAwaitable Loop::Schedule()
{
	return commandQueue.Schedule();
}

void Loop::RunAsync()
{
	asyncRunnerFlags = 0;
	asyncRunner =
		std::thread(+[](Loop *loop) { loop->_InternalSyncLoop(); }, this);
	asyncRunner.detach();
}

void Loop::SingleLoopIteration()
{
	stats.loopBigIterations+=1;
}

void Loop::_InternalSyncLoop()
{
	asyncRunnerFlags |= RUNNING;
	while (IsQueuedStopAsync() == false) {
		SingleLoopIteration();
		if (microsecondsOfSleepBetweenIterations > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(
				microsecondsOfSleepBetweenIterations));
		}
	}
	asyncRunnerFlags &= ~RUNNING;
}

void Loop::QueueStopRunning()
{
	asyncRunnerFlags |= QUEUE_STOP;
	if (IsRunningAsync()) {
		WakeUp();
	}
}

bool Loop::IsQueuedStopAsync() { return asyncRunnerFlags & QUEUE_STOP; }

bool Loop::IsRunningAsync() { return asyncRunnerFlags & RUNNING; }

void Loop::_InternalSingleLoopIteration()
{
	stats.loopSmallIterations+=1;
	commandQueue.Execute(1024);
	for (auto &h : hosts) {
		h->_InternalSingleLoopIteration();
	}
}

void Loop::SetSleepBetweenUnlockedIterations(int32_t microseconds)
{
	microsecondsOfSleepBetweenIterations = microseconds;
}
} // namespace icon7
