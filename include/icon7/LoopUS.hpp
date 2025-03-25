// Copyright (C) 2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_LOOP_US_TCP_HPP
#define ICON7_LOOP_US_TCP_HPP

#include <unordered_map>

#include "../../uSockets/src/libusockets.h"

#include "Loop.hpp"

namespace icon7
{
class Host;
class Peer;
namespace uS
{

namespace tcp
{
class Host;
class Peer;
} // namespace tcp

class Loop : public icon7::Loop
{
public:
	Loop();
	virtual ~Loop();

	Loop(Loop &) = delete;
	Loop(Loop &&) = delete;
	Loop(const Loop &) = delete;
	Loop &operator=(Loop &) = delete;
	Loop &operator=(Loop &&) = delete;
	Loop &operator=(const Loop &) = delete;

	bool Init(int timerWakeupRepeatMs);
	virtual void Destroy() override;

	virtual void SingleLoopIteration() override;

	virtual void WakeUp() override;

	std::shared_ptr<uS::tcp::Host> CreateHost(
		bool useSSL, const char *key_file_name = nullptr,
		const char *cert_file_name = nullptr, const char *passphrase = nullptr,
		const char *dh_params_file_name = nullptr,
		const char *ca_file_name = nullptr, const char *ssl_ciphers = nullptr);

	void _LocalSetTimerRepeat(int timerWakeupRepeatMs);
	void _LocalSetNoWaitLoop(bool value);

protected:
	static void _Internal_wakeup_cb(struct us_loop_t *loop);
	static void _Internal_pre_cb(struct us_loop_t *loop);
	static void _Internal_post_cb(struct us_loop_t *loop);

	static void _InternalOnTimerWakup(us_timer_t *timer);

	static Loop *LoopFromUsLoop(us_loop_t *loop);
	static Loop **LoopStoreFromUsLoop(us_loop_t *loop);

	friend class uS::tcp::Host;
	friend class uS::tcp::Peer;

protected:
	struct us_loop_t *loop;

	std::unordered_map<us_socket_context_t *, std::shared_ptr<uS::tcp::Host>>
		hostsBySocketContext;

	us_timer_t *timerWakeup;
	bool noWaitLoop = false;
};
} // namespace uS
} // namespace icon7

#endif
