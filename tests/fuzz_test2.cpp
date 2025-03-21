#include <random>

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

thread_local int64_t received = 0;

int Sum(int a, int b)
{
	received++;
	return a + b;
}

int Mul(int a, int b)
{
	received++;
	return a * b;
}

icon7::CoroutineSchedulable Test(icon7::uS::Loop *loop, const uint8_t *data,
								 size_t size)
{
	const int useSSL = (data[0] & 0) ? 1 : 0;
	thread_local uint16_t port = 0;

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

	thread_local std::random_device rd;
	thread_local std::mt19937_64 mt(rd());

	while (true) {
		while (port < 1025)
			port = mt() & 0x3FFF;

		auto listenFuture = host->ListenOnPort("127.0.0.1", port, icon7::IPv4);
		co_await loop->Schedule();

		if (listenFuture.get())
			break;
		port = 0;
	}

	auto manualPeer = host->ConnectPromise("127.0.0.1", port);
	co_await loop->Schedule();

	std::shared_ptr<icon7::Peer> peer = manualPeer.get();

	if (peer.get() == nullptr) {
		loop->QueueStopRunning();
	}

	icon7::ByteBuffer buffer(size + 256);
	buffer.append(data + 1, size - 1);
	peer->SendLocalThread(buffer);

	for (int i = 0; i < 15; ++i) {
		co_await loop->Schedule();
	}

	host->_InternalDestroy();
	host = nullptr;
	loop->QueueStopRunning();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 10) {
		return -1;
		LOG_FATAL("");
	}

	received = 0;
	icon7::Initialize();

	std::shared_ptr<icon7::uS::Loop> loop = std::make_shared<icon7::uS::Loop>();
	loop->Init(1);
	loop->SetSleepBetweenUnlockedIterations(10);
	loop->_LocalSetNoWaitLoop(true);

	Test(loop.get(), data, size);

	loop->_InternalSyncLoop();

	loop->Destroy();
	loop = nullptr;

	icon7::Deinitialize();
	return 0;
}
