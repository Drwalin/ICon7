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

#include <openssl/ssl.h>

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
	this->EnqueueCommand(
		commands::ExecuteOnHost{this, nullptr, [](icon7::Host *_host, void *) {
									Host *host = (Host *)_host;
									for (auto s : host->listenSockets) {
										us_listen_socket_close(host->SSL, s);
									}
								}});
	icon7::Host::_InternalDestroy();
}

bool Host::Init(bool useSSL, const char *key_file_name,
				const char *cert_file_name, const char *passphrase,
				const char *dh_params_file_name, const char *ca_file_name,
				const char *ssl_ciphers)
{
	SSL = useSSL;
	us_socket_context_options_t options{key_file_name, cert_file_name,
										passphrase,	   dh_params_file_name,
										ca_file_name,  ssl_ciphers};
	if (InitLoopAndContext(options) == false) {
		return false;
	}
	return true;
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
		DEBUG("");
		return false;
	}
	*(Host **)us_loop_ext(loop) = this;

	socketContext =
		us_create_socket_context(SSL, loop, sizeof(Host *), options);
	if (socketContext == nullptr) {
		us_loop_free(loop);
		DEBUG("");
		return false;
	}
	*(Host **)us_socket_context_ext(SSL, socketContext) = this;

	if (SSL) {
		SetUSocketContextCallbacks<true>();
	} else {
		SetUSocketContextCallbacks<false>();
	}

	return loop != nullptr;
}

void Host::_InternalListen(IPProto ipProto, uint16_t port,
						   commands::ExecuteBooleanOnHost &&com,
						   CommandExecutionQueue *queue)
{
	us_listen_socket_t *socket = nullptr;
	if (ipProto == IPv4) {
		socket = us_socket_context_listen(SSL, socketContext, "127.0.0.1", port,
										  0, sizeof(Host *));
	} else {
		socket = us_socket_context_listen(SSL, socketContext, "::1", port, 0,
										  sizeof(Host *));
	}
	if (socket) {
		listenSockets.insert(socket);
	}

	com.result = socket;
	if (queue) {
		queue->EnqueueCommand(std::move(com));
	} else {
		com.Execute();
	}
}

void Host::SingleLoopIteration()
{
	us_loop_run(loop);
	icon7::Host::SingleLoopIteration();
}

void Host::_Internal_wakeup_cb(struct us_loop_t *loop)
{
	auto host = (*(Host **)us_loop_ext(loop));
	host->icon7::Host::SingleLoopIteration();
}

void Host::_Internal_pre_cb(struct us_loop_t *loop)
{
	auto host = (*(Host **)us_loop_ext(loop));
	host->icon7::Host::SingleLoopIteration();
}

void Host::_Internal_post_cb(struct us_loop_t *loop)
{
	auto host = (*(Host **)us_loop_ext(loop));
	host->icon7::Host::SingleLoopIteration();
}

void Host::_InternalConnect(commands::ExecuteConnect &com)
{
	us_socket_t *socket =
		us_socket_context_connect(SSL, socketContext, com.address.c_str(),
								  com.port, nullptr, 0, sizeof(icon7::Peer *));

	std::shared_ptr<icon7::Peer> peer;
	if (socket) {
		peer = std::make_shared<uS::tcp::Peer>(this, socket);
		(*(icon7::Peer **)us_socket_ext(SSL, socket)) = peer.get();
		com.onConnected.peer = peer;
	} else {
		com.onConnected.peer = nullptr;
	}

	_InternalConnect_Finish(com);
}

template <bool SSL>
us_socket_t *Host::_Internal_on_open(struct us_socket_t *socket, int isClient,
									 char *ip, int ipLength)
{
	us_socket_context_t *context = us_socket_context(SSL, socket);
	us_loop_t *loop = us_socket_context_loop(SSL, context);
	Host *host = *(Host **)us_loop_ext(loop);

	std::shared_ptr<icon7::Peer> peer;

	if (isClient == false) {
		peer = std::make_shared<uS::tcp::Peer>(host, socket);
		(*(icon7::Peer **)us_socket_ext(SSL, socket)) = peer.get();
	} else {
		peer =
			(*(icon7::Peer **)us_socket_ext(SSL, socket))->shared_from_this();
	}

	host->_Internal_on_open_Finish(peer);

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_close(struct us_socket_t *socket, int code,
									  void *reason)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);
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

	peer->_InternalOnData((uint8_t *)data, length);
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_writable(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	peer->_InternalOnWritable();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_timeout(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	peer->_InternalOnTimeout();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_long_timeout(struct us_socket_t *socket)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	peer->_InternalOnLongTimeout();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_connect_error(struct us_socket_t *socket,
											  int code)
{
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);
	Host *host = (Host *)peer->host;

	peer->_InternalOnDisconnect();
	host->peers.erase(peer->shared_from_this());

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_end(struct us_socket_t *socket)
{
	return socket;
}

void Host::EnqueueCommand(Command &&command)
{
	icon7::Host::EnqueueCommand(std::move(command));
	us_wakeup_loop(loop);
}

} // namespace tcp
} // namespace uS
} // namespace icon7
