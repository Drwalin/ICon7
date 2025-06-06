// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_HOST_US_TCP_HPP
#define ICON7_HOST_US_TCP_HPP

#include <unordered_set>

#include "../../uSockets/src/libusockets.h"

#include "Host.hpp"

namespace icon7
{
namespace uS
{
class Loop;

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

	bool Init(std::shared_ptr<uS::Loop> loop, bool useSSL,
			  const char *key_file_name = nullptr,
			  const char *cert_file_name = nullptr,
			  const char *passphrase = nullptr,
			  const char *dh_params_file_name = nullptr,
			  const char *ca_file_name = nullptr,
			  const char *ssl_ciphers = nullptr);

	virtual void StopListening() override;

protected:
	virtual void _InternalConnect(
		commands::internal::ExecuteConnect &connectCommand) override;
	virtual void _InternalListen(
		const std::string &address, IPProto ipProto, uint16_t port,
		CommandHandle<commands::ExecuteBooleanOnHost> &com) override;

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

	virtual void _InternalStopListening() override;

	friend class uS::tcp::Peer;

protected:
	template <bool SSL>
	static Host *HostFromUsSocketContext(us_socket_context_t *context);

protected:
	int SSL;

	struct us_loop_t *uSloop;
	std::shared_ptr<uS::Loop> loop;

	us_socket_context_t *socketContext;
	std::unordered_set<us_listen_socket_t *> listenSockets;
};
} // namespace tcp
} // namespace uS
} // namespace icon7

#endif
