/*
 *  This file is part of ICon7.
 *  Copyright (C) 2025 Marek Zalewski aka Drwalin
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
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	// 	asyncRunner.join();
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
			while (loop->hosts.empty() == false) {
				auto h = loop->hosts.begin();
				(*h)->_InternalDestroy();
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

void Loop::_InternalSyncLoop()
{
	asyncRunnerFlags |= RUNNING;
	while (IsQueuedStopAsync() == false) {
		SingleLoopIteration();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
	commandQueue.Execute(1024);
	for (auto &h : hosts) {
		h->_InternalSingleLoopIteration();
	}
}

void Loop::WakeUp() {}
} // namespace icon7
