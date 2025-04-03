// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Command.hpp"
#include "../include/icon7/PeerUStcp.hpp"
#include "../include/icon7/LoopUS.hpp"

#include "../include/icon7/HostUStcp.hpp"

namespace icon7
{
namespace uS
{
namespace tcp
{
Host::Host()
{
	SSL = 0;
	socketContext = nullptr;
}

Host::~Host() {}

void Host::_InternalDestroy()
{
	icon7::Host::_InternalDestroy();

	us_socket_context_close(SSL, socketContext);
	loop->hostsBySocketContext.erase(socketContext);
	us_socket_context_free(SSL, socketContext);
	socketContext = nullptr;

	loop->hosts.erase(shared_from_this());
}

bool Host::Init(std::shared_ptr<uS::Loop> loop, bool useSSL,
				const char *key_file_name, const char *cert_file_name,
				const char *passphrase, const char *dh_params_file_name,
				const char *ca_file_name, const char *ssl_ciphers)
{
	SSL = useSSL;
	us_socket_context_options_t options{key_file_name, cert_file_name,
										passphrase,	   dh_params_file_name,
										ca_file_name,  ssl_ciphers};

	uSloop = loop->loop;
	socketContext =
		us_create_socket_context(SSL, loop->loop, sizeof(Host *), options);
	if (socketContext == nullptr) {
		LOG_ERROR("FAILED TO CREATE HOST");
		uSloop = nullptr;
		return false;
	} else {
		uS::tcp::Host::loop = loop;
		icon7::Host::loop = loop;
		commandQueue = &loop->commandQueue;
		loop->hosts.insert(shared_from_this());
	}
	*(Host **)us_socket_context_ext(SSL, socketContext) = this;

	if (SSL) {
		SetUSocketContextCallbacks<true>();
	} else {
		SetUSocketContextCallbacks<false>();
	}

	loop->hostsBySocketContext[socketContext] =
		std::dynamic_pointer_cast<uS::tcp::Host>(shared_from_this());

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
	Host *host = HostFromUsSocketContext<SSL>(context);

	host->stats.connectionsLocalTotal += 1;

	std::shared_ptr<icon7::Peer> peer;

	if (isClient == false) {
		peer = host->MakePeer(socket);
		(*(icon7::Peer **)us_socket_ext(SSL, socket)) = peer.get();
		peer->isClient = false;
		host->loop->stats.connectionsLocalTotal += 1;
	} else {
		peer =
			(*(icon7::Peer **)us_socket_ext(SSL, socket))->shared_from_this();
		host->loop->stats.connectionsRemoteTotal += 1;
	}

	peer->isClient = isClient;

	host->_Internal_on_open_Finish(peer);

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_close(struct us_socket_t *socket, int code,
									  void *reason)
{
	Host *host = HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
	host->stats.disconnectedTotal += 1;
	host->loop->stats.disconnectedTotal += 1;
	host->stats.disconnectedLocal += 1;
	host->loop->stats.disconnectedLocal += 1;

	// closed locally
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

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
		Host *host =
			HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
		host->stats.onWriteable += 1;
		host->loop->stats.onWriteable += 1;
		return socket;
	}

	peer->stats.onWriteable += 1;
	peer->host->stats.onWriteable += 1;
	peer->host->loop->stats.onWriteable += 1;

	peer->_InternalOnWritable();
	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_timeout(struct us_socket_t *socket)
{
	Host *host = HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
	host->stats.timeouts += 1;
	host->loop->stats.timeouts += 1;

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
	Host *host = HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
	host->stats.longTimeouts += 1;
	host->loop->stats.longTimeouts += 1;

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
	Host *host = HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
	host->stats.connectionsFailed += 1;
	host->loop->stats.connectionsFailed += 1;

	icon7::uS::tcp::Peer *peer =
		*(icon7::uS::tcp::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	peer->_InternalClearInternalDataOnClose();
	host->peers.erase(peer->shared_from_this());

	return socket;
}

template <bool SSL>
us_socket_t *Host::_Internal_on_end(struct us_socket_t *socket)
{
	Host *host = HostFromUsSocketContext<SSL>(us_socket_context(SSL, socket));
	host->stats.disconnectedTotal += 1;
	host->loop->stats.disconnectedTotal += 1;
	host->stats.disconnectedRemote += 1;
	host->loop->stats.disconnectedRemote += 1;

	// closed by the other side
	icon7::Peer *peer = *(icon7::Peer **)us_socket_ext(SSL, socket);

	if (peer == nullptr) {
		return socket;
	}

	std::shared_ptr<icon7::Peer> _peer = peer->shared_from_this();
	host->_Internal_on_close_Finish(_peer);

	us_socket_shutdown(SSL, socket);
	return us_socket_close(SSL, socket, 0, nullptr);
}

void Host::StopListening()
{
	class CommandStopListening final : public commands::ExecuteOnHost
	{
	public:
		CommandStopListening() {}
		virtual ~CommandStopListening() {}
		virtual void Execute() override { host->_InternalStopListening(); }
	};
	auto com = CommandHandle<CommandStopListening>::Create();
	com->host = this;
	EnqueueCommand(std::move(com));
}

void Host::_InternalStopListening()
{
	for (us_listen_socket_t *s : listenSockets) {
		us_listen_socket_close(SSL, s);
	}
	listenSockets.clear();
}

std::shared_ptr<Peer> Host::MakePeer(us_socket_t *socket)
{
	return std::make_shared<uS::tcp::Peer>(this, socket);
}

template <bool SSL>
Host *Host::HostFromUsSocketContext(us_socket_context_t *context)
{
	return *(Host **)us_socket_context_ext(SSL, context);
	/*
	us_loop_t *l = us_socket_context_loop(SSL, context);
	Loop *loop = *(Loop **)us_loop_ext(l);
	auto it = loop->hostsBySocketContext.find(context);
	if (it != loop->hostsBySocketContext.end()) {
		return it->second.get();
	}
	return nullptr;
	*/
}

} // namespace tcp
} // namespace uS
} // namespace icon7
