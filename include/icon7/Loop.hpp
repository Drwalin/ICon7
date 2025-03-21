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

#ifndef ICON7_LOOP_HPP
#define ICON7_LOOP_HPP

#include <memory>
#include <thread>
#include <unordered_set>

#include "CommandExecutionQueue.hpp"
#include "Forward.hpp"

namespace icon7
{
class Host;

class Loop : public std::enable_shared_from_this<Loop>
{
public:
	virtual ~Loop();

	Loop(Loop &) = delete;
	Loop(Loop &&) = delete;
	Loop(const Loop &) = delete;
	Loop &operator=(Loop &) = delete;
	Loop &operator=(Loop &&) = delete;
	Loop &operator=(const Loop &) = delete;

	virtual void Destroy();

	enum RunnerFlags : uint32_t { RUNNING = 1, QUEUE_STOP = 2 };

public: // multithreaded safe functions
	void RunAsync();
	void WaitStopRunning();
	void QueueStopRunning();
	bool IsRunningAsync();
	bool IsQueuedStopAsync();

	void SetSleepBetweenUnlockedIterations(int32_t microseconds);

	CommandExecutionQueue *GetCommandExecutionQueue() { return &commandQueue; }
	void EnqueueCommand(CommandHandle<Command> &&command);
	CommandExecutionQueue::CoroutineAwaitable
	Schedule(std::shared_ptr<void> &&obj);
	CommandExecutionQueue::CoroutineAwaitable Schedule();

	virtual void WakeUp() = 0;

public: // thread unsafe, safe only in hosts loop thread
	virtual void SingleLoopIteration() = 0;
	virtual void _InternalSingleLoopIteration();
	void _InternalSyncLoop();

public:
	uint64_t userData;
	void *userPointer;
	std::shared_ptr<void> userSmartPtr;

protected:
	Loop();

protected:
	std::unordered_set<std::shared_ptr<Host>> hosts;

	CommandExecutionQueue commandQueue;
	std::atomic<uint32_t> asyncRunnerFlags;
	std::thread asyncRunner;

	int32_t microsecondsOfSleepBetweenIterations = 1000;
};
} // namespace icon7

#endif
