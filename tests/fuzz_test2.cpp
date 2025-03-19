#include <cstdio>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/PeerUStcp.hpp"
#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/LoopUS.hpp"
#include "../include/icon7/CoroutineHelper.hpp"

std::atomic<int64_t> sent = 0, received = 0, returned = 0;

int Sum(int a, int b)
{
	LOG_FATAL("HIT SUM!!!!! SUCCESS");
	received++;
	return a + b;
}

int Mul(int a, int b)
{
	LOG_FATAL("HIT MUL!!!!! SECOND SUCCESS");
	received++;
	return a * b;
}

icon7::CoroutineSchedulable Test(icon7::uS::Loop *loop, const uint8_t *data,
								 size_t size)
{
	auto awaitable = loop->Schedule();
	if (awaitable.IsValid() == false) {
		LOG_FATAL("Failed????");
		co_return;
	}
	co_await awaitable;

	co_await loop->Schedule();

	const int useSSL = data[0] & 0;

	uint16_t port = 7312;

	icon7::RPCEnvironment rpc;
	rpc.RegisterMessage("sum", Sum);
	rpc.RegisterMessage("mul", Mul);
	std::shared_ptr<icon7::uS::tcp::Host> host =
		loop->CreateHost(useSSL, "../cert/user.key", "../cert/user.crt", "",
						 nullptr, "../cert/rootca.crt",
						 "ECDHE-ECDSA-AES256-GCM-SHA384:"
						 "ECDHE-ECDSA-AES128-GCM-SHA256:"
						 "ECDHE-ECDSA-CHACHA20-POLY1305:"
						 "DHE-RSA-AES256-GCM-SHA384:");
	host->SetRpcEnvironment(&rpc);
	co_await loop->Schedule();

	while (true) {
		port++;
		if (port < 1025) {
			port = 1025;
		}

		auto listenFuture = host->ListenOnPort("127.0.0.1", port, icon7::IPv4);
		co_await loop->Schedule();

		if (listenFuture.get()) {
			break;
		}
	}

	auto manualPeer = host->ConnectPromise("127.0.0.1", port);
	co_await loop->Schedule();

	std::shared_ptr<icon7::Peer> peer = manualPeer.get();
	co_await loop->Schedule();
	
	icon7::ByteBuffer buffer(size+256);
	buffer.append(data+1, size-1);
	peer->Send(buffer);
	
	for(int i=0; i<10 && received == 0; ++i) {
		co_await loop->Schedule();
	}

	host->_InternalDestroy();
	host = nullptr;
	loop->QueueStopRunning();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 64)
		return 0;

	icon7::Initialize();

	std::shared_ptr<icon7::uS::Loop> loop = std::make_shared<icon7::uS::Loop>();
	loop->Init(1);

	Test(loop.get(), data, size);

	loop->_InternalSyncLoop();

	loop->Destroy();
	loop = nullptr;
	
	icon7::Deinitialize();
	return 0;
}
