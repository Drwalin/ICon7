// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Debug.hpp"

#include "../include/icon7/OnReturnCallback.hpp"

namespace icon7
{
bool OnReturnCallback::IsExpired(int64_t t) const
{
	return t > timeoutTimePoint;
}

void OnReturnCallback::Execute(Peer *peer, Flags flags, ByteReader &reader)
{
	if (peer != callback->peer.get()) {
		LOG_WARN(
			"OnReturnedCallback executed by different peer than the request "
			"was sent to");
	}

	callback->flags = flags;
	callback->reader = std::move(reader);
	callback->timedOut = false;
	if (executionQueue) {
		executionQueue->EnqueueCommand(std::move(callback));
	} else {
		callback.Execute();
	}
}

void OnReturnCallback::ExecuteTimeout()
{
	callback->timedOut = true;
	if (executionQueue) {
		executionQueue->EnqueueCommand(std::move(callback));
	} else {
		callback.Execute();
	}
}
} // namespace icon7
