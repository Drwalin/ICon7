// Copyright (C) 2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/HostUStcp.hpp"

#include "../include/icon7/LoopUS.hpp"

namespace icon7
{
namespace uS
{
Loop::Loop() {}

Loop::~Loop() {}

void Loop::Destroy()
{
	icon7::Loop::Destroy();

	us_loop_run_once(loop);
	commandQueue.Execute(128);
	us_loop_run_once(loop);
	do {
		commandQueue.Execute(128);
		us_loop_run_once(loop);
	} while (commandQueue.HasAny());

	us_timer_close(timerWakeup);
	timerWakeup = nullptr;
	us_loop_free(loop);
	loop = nullptr;
}

bool Loop::Init(int timerWakeupRepeatMs)
{
	loop = us_create_loop(nullptr, _Internal_wakeup_cb, _Internal_pre_cb,
						  _Internal_post_cb, sizeof(Loop *));
	if (loop == nullptr) {
		LOG_ERROR("FAILED TO CREATE LOOP");
		return false;
	}
	*LoopStoreFromUsLoop(loop) = this;

	timerWakeup = us_create_timer(loop, true, sizeof(void *));
	*(Loop **)us_timer_ext(timerWakeup) = this;
	_LocalSetTimerRepeat(timerWakeupRepeatMs);
	return true;
}

std::shared_ptr<uS::tcp::Host>
Loop::CreateHost(bool useSSL, const char *key_file_name,
				 const char *cert_file_name, const char *passphrase,
				 const char *dh_params_file_name, const char *ca_file_name,
				 const char *ssl_ciphers)
{
	std::shared_ptr<uS::tcp::Host> host = std::make_shared<uS::tcp::Host>();
	if (host->Init(
			std::dynamic_pointer_cast<uS::Loop>(this->shared_from_this()),
			useSSL, key_file_name, cert_file_name, passphrase,
			dh_params_file_name, ca_file_name, ssl_ciphers)) {
		return host;
	}
	host = nullptr;
	return nullptr;
}

void Loop::_LocalSetTimerRepeat(int timerWakeupRepeatMs)
{
	us_timer_set(timerWakeup, _InternalOnTimerWakup, timerWakeupRepeatMs,
				 timerWakeupRepeatMs);
}

void Loop::_InternalOnTimerWakup(us_timer_t *timer)
{
	Loop *loop = *(Loop **)us_timer_ext(timer);
	loop->stats.loopTimerWakeups += 1;
	loop->_InternalSingleLoopIteration();
}

void Loop::SingleLoopIteration()
{
	icon7::Loop::SingleLoopIteration();
	if (noWaitLoop) {
		us_loop_run_once(loop);
	} else {
		us_loop_run(loop);
	}
	_InternalSingleLoopIteration();
}

void Loop::_Internal_wakeup_cb(struct us_loop_t *loop)
{
	Loop *_loop = LoopFromUsLoop(loop);
	_loop->stats.loopWakeups += 1;
	_loop->_InternalSingleLoopIteration();
}

void Loop::_Internal_pre_cb(struct us_loop_t *loop)
{
	Loop *_loop = LoopFromUsLoop(loop);
	_loop->_InternalSingleLoopIteration();
}

void Loop::_Internal_post_cb(struct us_loop_t *loop)
{
	Loop *_loop = LoopFromUsLoop(loop);
	if (!_loop->noWaitLoop) {
		_loop->_InternalSingleLoopIteration();
	}
}

void Loop::WakeUp() { us_wakeup_loop(loop); }

void Loop::_LocalSetNoWaitLoop(bool value) { noWaitLoop = value; }

Loop *Loop::LoopFromUsLoop(us_loop_t *loop)
{
	return *LoopStoreFromUsLoop(loop);
}

Loop **Loop::LoopStoreFromUsLoop(us_loop_t *loop)
{
	return (Loop **)us_loop_ext(loop);
}

} // namespace uS
} // namespace icon7
