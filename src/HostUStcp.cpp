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
#include "../include/icon7/Command.hpp"
#include "../include/icon7/PeerUStcp.hpp"

#include "../include/icon7/HostUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcp
{
Host::Host()
{
	SSL = -1;
	loop = nullptr;
	socketContext = nullptr;
}

Host::~Host() {}

void Host::_InternalDestroy()
{
	class CommandCloseAllSockets final : public commands::ExecuteOnHost
	{
	public:
		CommandCloseAllSockets() {}
		virtual ~CommandCloseAllSockets() {}
		virtual void Execute() override
		{
			Host *host = (Host *)(this->host);
			// close all sockets
			us_socket_context_close(host->SSL, host->socketContext);
		}
	};
	auto com = CommandHandle<CommandCloseAllSockets>::Create();
	com->host = this;
	EnqueueCommand(std::move(com));

	icon7::Host::_InternalDestroy();

	us_socket_context_free(SSL, socketContext);
	socketContext = nullptr;

	us_timer_close(timerWakeup);
	timerWakeup = nullptr;

	us_loop_free(loop);
	loop = nullptr;
}

bool Host::Init(bool useSSL, const char *key_file_name,
				const char *cert_file_name, const char *passphrase,
				const char *dh_params_file_name, const char *ca_file_name,
				const char *ssl_ciphers, int timerWakeupRepeatMs)
{
	SSL = useSSL;
	us_socket_context_options_t options{key_file_name, cert_file_name,
										passphrase,	   dh_params_file_name,
										ca_file_name,  ssl_ciphers};
	if (InitLoopAndContext(options) == false) {
		return false;
	}
	timerWakeup = us_create_timer(loop, true, sizeof(void *));
	*(Host **)us_timer_ext(timerWakeup) = this;
	_LocalSetTimerRepeat(timerWakeupRepeatMs);
	return true;
}

void Host::_LocalSetTimerRepeat(int timerWakeupRepeatMs)
{
	us_timer_set(timerWakeup, _InternalOnTimerWakup, timerWakeupRepeatMs,
				 timerWakeupRepeatMs);
}

void Host::_InternalOnTimerWakup(us_timer_t *timer)
{
	Host *host = *(Host **)us_timer_ext(timer);
	host->_InternalSingleLoopIteration(false);
}

template <bool _SSL> void Host::SetUSocketContextCallbacks()
{
	us_socket_context_on_open(_SSL, socketContext,
							  Host::_Internal_on_open<_SSL>);
	us_socket_context_on_close(_SSL, socketContext,
							   Host::_Internal_on_close<_SSL>);
	us_socket_context_on_data(_SSL, socketContext,
							  Host::_Internal_on_data<_SSL>);
	us_socket_context_on_writable(_SSL, socketContext,
								  Host::_Internal_on_writable<_SSL>);
	us_socket_context_on_timeout(_SSL, socketContext,
								 Host::_Internal_on_timeout<_SSL>);
	us_socket_context_on_long_timeout(_SSL, socketContext,
									  Host::_Internal_on_long_timeout<_SSL>);
	us_socket_context_on_connect_error(_SSL, socketContext,
									   Host::_Internal_on_connect_error<_SSL>);
	us_socket_context_on_end(_SSL, socketContext, Host::_Internal_on_end<_SSL>);
}

bool Host::InitLoopAndContext(us_socket_context_options_t options)
{
	loop = us_create_loop(nullptr, _Internal_wakeup_cb, _Internal_pre_cb,
						  _Internal_post_cb, sizeof(Host *));
	if (loop == nullptr) {
		LOG_ERROR("FAILED TO CREATE LOOP");
		return false;
	}
	*HostStoreFromUsLoop(loop) = this;

	socketContext =
		us_create_socket_context(SSL, loop, sizeof(Host *), options);
	if (socketContext == nullptr) {
		LOG_ERROR("FAILED TO CREATE HOST");
		us_loop_free(loop);
		loop = nullptr;
		return false;
	}
	*(Host **)us_socket_context_ext(SSL, socketContext) = this;

	if (SSL) {
		SetUSocketContextCallbacks<true>();
	} else {
		SetUSocketContextCallbacks<false>();
	}

	return true;
}

void Host::_InternalListen(const std::string &address, IPProto ipProto,
						   uint16_t port,
						   CommandHandle<commands::ExecuteBooleanOnHost> &com)
{
	us_listen_socket_t *socket = nullptr;
	if (address.size() == 0) {
		if (ipProto == IPv4) {
			socket = us_socket_context_listen(SSL, socketContext, "127.0.0.1",
											  port, 0, sizeof(Host *));
		} else {
			socket = us_socket_context_listen(SSL, socketContext, "::1", port,
											  0, sizeof(Host *));
		}
	} else {
		socket = us_socket_context_listen(SSL, socketContext, address.c_str(),
										  port, 0, sizeof(Host *));
	}
	if (socket) {
		listenSockets.insert(socket);
	}

	com->result = socket;
}

void Host::SingleLoopIteration()
{
	us_loop_run(loop);
	_InternalSingleLoopIteration(true);
}

void Host::_Internal_wakeup_cb(struct us_loop_t *loop)
{
	Host *host = HostFromUsLoop(loop);
	host->_InternalSingleLoopIteration(true);
}

void Host::_Internal_pre_cb(struct us_loop_t *loop)
{
	Host *host = HostFromUsLoop(loop);
	host->_InternalSingleLoopIteration(false);
}

void Host::_Internal_post_cb(struct us_loop_t *loop)
{
	Host *host = HostFromUsLoop(loop);
	host->_InternalSingleLoopIteration(false);
}

void Host::_InternalConnect(commands::internal::ExecuteConnect &com)
{
	us_socket_t *socket =
		us_socket_context_connect(SSL, socketContext, com.address.c_str(),
								  com.port, nullptr, 0, sizeof(icon7::Peer *));

	std::shared_ptr<icon7::Peer> peer;
	if (socket) {
		peer = MakePeer(socket);
		(*(icon7::Peer **)us_socket_ext(SSL, socket)) = peer.get();
		com.onConnected->peer = peer;
		peer->isClient = true;
	} else {
		com.onConnected->peer = nullptr;
	}

	_InternalConnect_Finish(com);
}

template <bool SSL>
us_socket_t *Host::_Internal_on_open(struct us_socket_t *socket, int isClient,
									 char *ip, int ipLength)
{
	us_socket_context_t *context = us_socket_context(SSL, socket);
	us_loop_t *loop = us_socket_context_loop(SSL, context);
	Host *host = HostFromUsLoop(loop);

	std::shared_ptr<icon7::Peer> peer;

	if (isClient == false) {
		peer = host->MakePeer(socket);
		(*(icon7::Peer **)us_socket_ext(SSL, socket)) = peer.get();
		peer->isClient = false;
	} else {
		peer =
			(*(icon7::Peer **)us_socket_ext(SSL, socket))->shared_from_this();
	}

	peer->isClient = isClient;

	host->_Internal_on_open_Finish(peer);

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_close(struct us_socket_t *socket, int code,
									  void *reason)
{
	// closed locally
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	// at the following line, sometimes error happens due to invalid peer
	// pointer
	Host *host = (Host *)peer->host;

	std::shared_ptr<icon7::Peer> _peer = peer->shared_from_this();
	host->_Internal_on_close_Finish(_peer);

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_data(struct us_socket_t *socket, char *data,
									 int length)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	peer->_InternalOnData((uint8_t *)data, length);
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_writable(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	peer->_InternalOnWritable();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_timeout(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer) {
		peer->_InternalOnTimeout();
	}

	if (!us_socket_is_established(SSL, socket) || peer == nullptr) {
		us_socket_shutdown(SSL, socket);
		return us_socket_close(SSL, socket, 0, nullptr);
	}

	if (us_socket_is_shut_down(SSL, socket)) {
		return us_socket_close(SSL, socket, 0, nullptr);
	}

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_long_timeout(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	peer->_InternalOnLongTimeout();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_connect_error(struct us_socket_t *socket,
											  int code)
{
	icon7::uS::tcp::Peer *peer =
		*(icon7::uS::tcp::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	Host *host = (Host *)peer->host;

	peer->_InternalClearInternalDataOnClose();
	host->peers.erase(peer->shared_from_this());

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_end(struct us_socket_t *socket)
{
	// closed by the other side
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	Host *host = (Host *)peer->host;

	std::shared_ptr<icon7::Peer> _peer = peer->shared_from_this();
	host->_Internal_on_close_Finish(_peer);

	us_socket_shutdown(SSL, socket);
	return us_socket_close(SSL, socket, 0, nullptr);
}

void Host::WakeUp() { us_wakeup_loop(loop); }

void Host::StopListening()
{
	class CommandStopListening final : public commands::ExecuteOnHost
	{
	public:
		CommandStopListening() {}
		virtual ~CommandStopListening() {}
		virtual void Execute() override
		{
			for (us_listen_socket_t *s : ((tcp::Host *)host)->listenSockets) {
				us_listen_socket_close(((tcp::Host *)host)->SSL, s);
			}
			((tcp::Host *)host)->listenSockets.clear();
		}
	};
	auto com = CommandHandle<CommandStopListening>::Create();
	com->host = this;
	EnqueueCommand(std::move(com));
}

std::shared_ptr<Peer> Host::MakePeer(us_socket_t *socket)
{
	return std::make_shared<uS::tcp::Peer>(this, socket);
}

Host *Host::HostFromUsLoop(us_loop_t *loop)
{
	return *HostStoreFromUsLoop(loop);
}

Host **Host::HostStoreFromUsLoop(us_loop_t *loop)
{
	return (Host **)us_loop_ext(loop);
}

} // namespace tcp
} // namespace uS
} // namespace icon7
