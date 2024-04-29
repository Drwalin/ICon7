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

#ifndef ICON7_ON_RETURN_CALLBACK_HPP
#define ICON7_ON_RETURN_CALLBACK_HPP

#include "Host.hpp"
#include "Peer.hpp"

namespace icon7
{

class ExecuteReturnCallbackBase : public commands::ExecuteOnPeer
{
public:
	ExecuteReturnCallbackBase() = default;
	virtual ~ExecuteReturnCallbackBase() = default;

	ByteReader reader;
	Flags flags = 0;
	bool timedOut = false;
};

template <typename Tret>
class ExecuteReturnCallback : public ExecuteReturnCallbackBase
{
public:
	ExecuteReturnCallback() = default;
	virtual ~ExecuteReturnCallback() = default;

	virtual void Execute() override final
	{
		if (timedOut) {
			ExecuteTimeout();
		} else {
			Tret v;
			reader.op(v);
			ExecuteOnReturn(v);
		}
	}
	virtual void ExecuteOnReturn(Tret &v) = 0;
	virtual void ExecuteTimeout() = 0;
};

template <> class ExecuteReturnCallback<void> : public ExecuteReturnCallbackBase
{
public:
	ExecuteReturnCallback() = default;
	virtual ~ExecuteReturnCallback() = default;

	virtual void Execute() override final
	{
		if (timedOut) {
			ExecuteTimeout();
		} else {
			ExecuteOnReturn();
		}
	}
	virtual void ExecuteOnReturn() = 0;
	virtual void ExecuteTimeout() = 0;
};

template <typename Tret>
class ExecuteReturnCallbackStdfunction final
	: public ExecuteReturnCallback<Tret>
{
public:
	ExecuteReturnCallbackStdfunction() = default;
	virtual ~ExecuteReturnCallbackStdfunction() = default;

	std::function<void(Peer *, Flags, Tret &)> onReturned;
	std::function<void(Peer *)> onTimeout;

	virtual void ExecuteOnReturn(Tret &v) override
	{
		onReturned(this->peer.get(), this->flags, v);
	}

	virtual void ExecuteTimeout() override
	{
		if (onTimeout)
			onTimeout(this->peer.get());
	}
};

template <>
class ExecuteReturnCallbackStdfunction<void> final
	: public ExecuteReturnCallback<void>
{
public:
	ExecuteReturnCallbackStdfunction() = default;
	virtual ~ExecuteReturnCallbackStdfunction() = default;

	std::function<void(Peer *, Flags)> onReturned;
	std::function<void(Peer *)> onTimeout;

	virtual void ExecuteOnReturn() override
	{
		if (onReturned)
			onReturned(this->peer.get(), this->flags);
	}

	virtual void ExecuteTimeout() override
	{
		if (onTimeout)
			onTimeout(this->peer.get());
	}
};

class OnReturnCallback
{
public:
	~OnReturnCallback() = default;

	OnReturnCallback() = default;
	OnReturnCallback(OnReturnCallback &&) = default;

	OnReturnCallback(OnReturnCallback &) = delete;
	OnReturnCallback(const OnReturnCallback &) = delete;

	OnReturnCallback &operator=(OnReturnCallback &&) = default;

	OnReturnCallback &operator=(OnReturnCallback &) = delete;
	OnReturnCallback &operator=(const OnReturnCallback &) = delete;

	bool IsExpired(std::chrono::time_point<std::chrono::steady_clock> t =
					   std::chrono::steady_clock::now()) const;
	void Execute(Peer *peer, Flags flags, ByteReader &reader);
	void ExecuteTimeout();

private:
public:
	template <typename Tret, typename Tfunc, typename Ttimeout>
	static OnReturnCallback
	Make(Tfunc &&onReturned, Ttimeout &&onTimeout, uint32_t timeoutMilliseconds,
		 Peer *peer, CommandExecutionQueue *executionQueue = nullptr)
	{
		OnReturnCallback ret;
		auto cb =
			CommandHandle<ExecuteReturnCallbackStdfunction<Tret>>::Create();
		cb->peer = peer->shared_from_this();
		cb->onReturned = std::move(onReturned);
		cb->onTimeout = std::move(onTimeout);
		ret.callback = std::move(cb);
		ret.executionQueue = executionQueue;
		ret.timeoutTimePoint = std::chrono::steady_clock::now() +
							   (std::chrono::milliseconds(timeoutMilliseconds));
		return ret;
	}

	template <typename Tfunc, typename Ttimeout>
	static OnReturnCallback
	Make(Tfunc &&onReturned, Ttimeout &&onTimeout, uint32_t timeoutMilliseconds,
		 Peer *peer, CommandExecutionQueue *executionQueue = nullptr)
	{
		OnReturnCallback ret;
		auto cb =
			CommandHandle<ExecuteReturnCallbackStdfunction<void>>::Create();
		cb->peer = peer->shared_from_this();
		cb->onReturned = std::move(onReturned);
		cb->onTimeout = std::move(onTimeout);
		ret.callback = std::move(cb);
		ret.executionQueue = executionQueue;
		ret.timeoutTimePoint = std::chrono::steady_clock::now() +
							   (std::chrono::milliseconds(timeoutMilliseconds));
		return ret;
	}

public:
	CommandHandle<ExecuteReturnCallbackBase> callback;
	CommandExecutionQueue *executionQueue;
	std::chrono::time_point<std::chrono::steady_clock> timeoutTimePoint;
};
} // namespace icon7

#endif
