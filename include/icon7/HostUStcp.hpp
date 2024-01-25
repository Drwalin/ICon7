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

#ifndef ICON7_HOST_US_TCP_HPP
#define ICON7_HOST_US_TCP_HPP

#include "../uSockets/src/libusockets.h"

#include "Host.hpp"

namespace icon7
{
namespace uS
{
namespace tcp
{
class Peer;

class Host : public icon7::Host
{
public:
	Host();
	virtual ~Host();

	Host(Host &) = delete;
	Host(Host &&) = delete;
	Host(const Host &) = delete;
	Host &operator=(Host &) = delete;
	Host &operator=(Host &&) = delete;
	Host &operator=(const Host &) = delete;

	virtual void _InternalDestroy() override;

	bool Init(bool useSSL = false, const char *key_file_name = nullptr,
			  const char *cert_file_name = nullptr,
			  const char *passphrase = nullptr,
			  const char *dh_params_file_name = nullptr,
			  const char *ca_file_name = nullptr,
			  const char *ssl_ciphers = nullptr);

	virtual void SingleLoopIteration() override;

	virtual void WakeUp() override;

	virtual void StopListening() override;

protected:
	bool InitLoopAndContext(us_socket_context_options_t options);

	virtual void
	_InternalConnect(commands::ExecuteConnect &connectCommand) override;
	virtual void _InternalListen(IPProto ipProto, uint16_t port,
								 commands::ExecuteBooleanOnHost &com) override;

	static void _Internal_wakeup_cb(struct us_loop_t *loop);
	static void _Internal_pre_cb(struct us_loop_t *loop);
	static void _Internal_post_cb(struct us_loop_t *loop);

	template <bool SSL>
	static us_socket_t *_Internal_on_open(struct us_socket_t *socket,
										  int is_client, char *ip,
										  int ip_length);
	template <bool SSL>
	static us_socket_t *_Internal_on_close(struct us_socket_t *socket, int code,
										   void *reason);
	template <bool SSL>
	static us_socket_t *_Internal_on_data(struct us_socket_t *socket,
										  char *data, int length);
	template <bool SSL>
	static us_socket_t *_Internal_on_writable(struct us_socket_t *socket);
	template <bool SSL>
	static us_socket_t *_Internal_on_timeout(struct us_socket_t *socket);
	template <bool SSL>
	static us_socket_t *_Internal_on_long_timeout(struct us_socket_t *socket);
	template <bool SSL>
	static us_socket_t *_Internal_on_connect_error(struct us_socket_t *socket,
												   int code);
	template <bool SSL>
	static us_socket_t *_Internal_on_end(struct us_socket_t *socket);

	template <bool _SSL> void SetUSocketContextCallbacks();

	virtual std::shared_ptr<icon7::uS::tcp::Peer> MakePeer(us_socket_t *socket);

	friend class Peer;

protected:
	int SSL;

	struct us_loop_t *loop;

	us_socket_context_t *socketContext;
	std::unordered_set<us_listen_socket_t *> listenSockets;
};
} // namespace tcp
} // namespace uS
} // namespace icon7

#endif
