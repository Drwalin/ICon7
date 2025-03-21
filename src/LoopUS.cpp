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
	Loop *host = *(Loop **)us_timer_ext(timer);
	host->_InternalSingleLoopIteration();
}

void Loop::SingleLoopIteration()
{
	us_loop_run(loop);
	_InternalSingleLoopIteration();
}

void Loop::_Internal_wakeup_cb(struct us_loop_t *loop)
{
	Loop *host = LoopFromUsLoop(loop);
	host->_InternalSingleLoopIteration();
}

void Loop::_Internal_pre_cb(struct us_loop_t *loop)
{
	Loop *host = LoopFromUsLoop(loop);
	host->_InternalSingleLoopIteration();
}

void Loop::_Internal_post_cb(struct us_loop_t *loop)
{
	Loop *host = LoopFromUsLoop(loop);
	host->_InternalSingleLoopIteration();
}

void Loop::WakeUp() { us_wakeup_loop(loop); }

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
