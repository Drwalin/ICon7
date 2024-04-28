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

#include "../include/icon7/Debug.hpp"

#include "../include/icon7/OnReturnCallback.hpp"

namespace icon7
{
bool OnReturnCallback::IsExpired(
	std::chrono::time_point<std::chrono::steady_clock> t) const
{
	return t > timeoutTimePoint;
}

void OnReturnCallback::Execute(Peer *peer, Flags flags, ByteReader &reader)
{
	if (peer != this->peer.get()) {
		LOG_WARN(
			"OnReturnedCallback executed by different peer than the request "
			"was sent to");
	}

	if (executionQueue) {
		auto com = CommandHandle<commands::internal::ExecuteReturnRC>::Create(
			std::move(reader));
		com->peer = this->peer;
		com->function = _internalOnReturnedValue;
		com->funcPtr = onReturned;
		com->flags = flags;
		executionQueue->EnqueueCommand(std::move(com));
	} else {
		_internalOnReturnedValue(this->peer.get(), flags, reader, onReturned);
	}
}

void OnReturnCallback::ExecuteTimeout()
{
	if (onTimeout) {
		if (executionQueue) {
			auto com = CommandHandle<commands::ExecuteOnPeerNoArgs>();
			com->peer = peer;
			com->function = onTimeout;
			executionQueue->EnqueueCommand(std::move(com));
		} else {
			onTimeout(peer.get());
		}
	}
}
} // namespace icon7
