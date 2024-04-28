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

#ifndef ICON7_COMMAND_EXECUTION_QUEUE_HPP
#define ICON7_COMMAND_EXECUTION_QUEUE_HPP

#include <atomic>

#include "../../concurrent/mpsc_queue.hpp"

#include "Command.hpp"

namespace icon7
{
class CommandExecutionQueue
{
public:
	CommandExecutionQueue();
	~CommandExecutionQueue();

	void EnqueueCommand(CommandHandle<Command> &&command);
	bool TryDequeue(CommandHandle<Command> &command);

	void QueueStopAsyncExecution();
	void WaitStopAsyncExecution();
	bool IsRunningAsync() const;

	bool HasAny() const;

	void RunAsyncExecution(uint32_t sleepMicrosecondsOnNoActions);
	void ExecuteLoop(uint32_t sleepMicrosecondsOnNoActions);
	uint32_t Execute(uint32_t maxToDequeue);

private:
	enum AsyncExecutionFlags {
		STARTING_RUNNING = 1,
		IS_RUNNING = 2,
		QUEUE_STOP = 4,
		STOPPED = 0
	};

	std::atomic<uint32_t> asyncExecutionFlags;

private:
	concurrent::mpsc::queue<Command> queue;
};
} // namespace icon7

#endif
