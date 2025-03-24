// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_ON_RETURN_CALLBACK_HPP
#define ICON7_ON_RETURN_CALLBACK_HPP

#include "Command.hpp"
#include "Host.hpp"
#include "Peer.hpp"

namespace icon7
{

class ExecuteReturnCallbackBase : public commands::ExecuteOnPeer
{
public:
	ExecuteReturnCallbackBase() {}
	virtual ~ExecuteReturnCallbackBase() {}

	ByteReader reader;
	Flags flags = 0;
	bool timedOut = false;
};

template <typename Tret>
class ExecuteReturnCallback : public ExecuteReturnCallbackBase
{
public:
	ExecuteReturnCallback() {}
	virtual ~ExecuteReturnCallback() {}

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
	ExecuteReturnCallback() {}
	virtual ~ExecuteReturnCallback() {}

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
	ExecuteReturnCallbackStdfunction() {}
	virtual ~ExecuteReturnCallbackStdfunction() {}

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
	ExecuteReturnCallbackStdfunction() {}
	virtual ~ExecuteReturnCallbackStdfunction() {}

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
	~OnReturnCallback() {}

	OnReturnCallback() {}
	OnReturnCallback(OnReturnCallback &&o)
		: callback(std::move(o.callback)), executionQueue(o.executionQueue),
		  timeoutTimePoint(o.timeoutTimePoint), pointerHolder(o.pointerHolder)
	{
		o.executionQueue = nullptr;
		o.pointerHolder = nullptr;
	}

	OnReturnCallback(OnReturnCallback &) = delete;
	OnReturnCallback(const OnReturnCallback &) = delete;

	OnReturnCallback &operator=(OnReturnCallback &&o)
	{
		callback = std::move(o.callback);
		executionQueue = o.executionQueue;
		timeoutTimePoint = o.timeoutTimePoint;
		o.executionQueue = nullptr;
		pointerHolder = o.pointerHolder;
		o.pointerHolder = nullptr;
		return *this;
	}

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
		 Peer *peer, CommandExecutionQueue *executionQueue = nullptr,
		 std::shared_ptr<void> pointerHolder = nullptr)
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
		ret.pointerHolder = pointerHolder;
		return ret;
	}

	template <typename Tfunc, typename Ttimeout>
	static OnReturnCallback
	Make(Tfunc &&onReturned, Ttimeout &&onTimeout, uint32_t timeoutMilliseconds,
		 Peer *peer, CommandExecutionQueue *executionQueue = nullptr,
		 std::shared_ptr<void> pointerHolder = nullptr)
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
		ret.pointerHolder = pointerHolder;
		return ret;
	}

public:
	CommandHandle<ExecuteReturnCallbackBase> callback;
	CommandExecutionQueue *executionQueue;
	std::chrono::time_point<std::chrono::steady_clock> timeoutTimePoint;
	std::shared_ptr<void> pointerHolder;
};
} // namespace icon7

#endif
