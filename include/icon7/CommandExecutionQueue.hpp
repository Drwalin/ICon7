// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_COMMAND_EXECUTION_QUEUE_HPP
#define ICON7_COMMAND_EXECUTION_QUEUE_HPP

#include <atomic>
#include <memory>
#include <coroutine>

#include "Forward.hpp"

namespace icon7
{
class CommandExecutionQueue
{
public:
	CommandExecutionQueue();
	~CommandExecutionQueue();

	void EnqueueCommandsBuffer(CommandsBuffer &&commands);
	void EnqueueCommand(CommandHandle<Command> &&command);
	bool TryDequeue(CommandHandle<Command> &command);
	size_t TryDequeueBulk(CommandHandle<Command> *commands, size_t max);

	void QueueStopAsyncExecution();
	void WaitStopAsyncExecution();
	bool IsRunningAsync() const;

	bool HasAny() const;

	void RunAsyncExecution(uint32_t sleepMicrosecondsOnNoActions,
						   uint32_t maxSleepDuration);
	void ExecuteLoop(uint32_t sleepMicrosecondsOnNoActions,
					 uint32_t maxSleepDuration);
	uint32_t Execute(uint32_t maxToDequeue);

public:
	struct CoroutineAwaitable {
		inline CoroutineAwaitable(CommandExecutionQueue *queue,
								  std::shared_ptr<void> &obj)
			: queue(queue), objectHolder(obj)
		{
		}
		inline CoroutineAwaitable(CommandExecutionQueue *queue,
								  std::shared_ptr<void> &&obj)
			: queue(queue), objectHolder(std::move(obj))
		{
		}
		inline CoroutineAwaitable() : queue(nullptr), objectHolder(nullptr) {}
		inline CoroutineAwaitable(CoroutineAwaitable &&o)
			: queue(o.queue), objectHolder(std::move(o.objectHolder))
		{
			o.queue = nullptr;
			o.objectHolder = nullptr;
		}

		inline CoroutineAwaitable &operator=(CoroutineAwaitable &&o)
		{
			this->queue = o.queue;
			this->objectHolder = std::move(o.objectHolder);
			o.queue = nullptr;
			o.objectHolder = nullptr;
			return *this;
		}

		inline bool await_ready() { return false; }
		inline void await_resume() {}
		void await_suspend(std::coroutine_handle<> h);

		CommandExecutionQueue *queue = nullptr;
		std::shared_ptr<void> objectHolder;

		inline bool IsValid() const { return queue; }
	};
	inline CoroutineAwaitable Schedule(std::shared_ptr<void> &&obj)
	{
		return CoroutineAwaitable(this, std::move(obj));
	}
	inline CoroutineAwaitable Schedule()
	{
		return CoroutineAwaitable(this, std::shared_ptr<void>(userSmartPtr));
	}

public:
	void *userPtr = nullptr;
	std::shared_ptr<void> userSmartPtr;
	uint64_t userData = 0;

private:
	enum AsyncExecutionFlags {
		STARTING_RUNNING = 1,
		IS_RUNNING = 2,
		QUEUE_STOP = 4,
		STOPPED = 0
	};

	std::atomic<uint32_t> asyncExecutionFlags;

	friend class Host;

private:
	moodycamel::ConcurrentQueue<CommandHandle<Command>,
								ConcurrentQueueDefaultTraits> *queue;
};
} // namespace icon7

#endif
