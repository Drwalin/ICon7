#include "icon7/Command.hpp"
#include <cstdio>

#include <iostream>
#include <chrono>
#include <thread>

#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

icon7::RPCEnvironment rpc;

int Sum(int a, int b)
{
	printf("----------------------------------- sum %i + %i = %i\n", a, b,
		   a + b);
	return a + b;
}

int Mul(int a, int b)
{
	printf("----------------------------------- mul %i + %i = %i\n", a, b,
		   a * b);
	return a * b;
}

int main()
{
	uint16_t port = 7312;

	icon7::Initialize();

	rpc.RegisterMessage("sum", Sum);
	rpc.RegisterMessage("mul", Mul);

	icon7::HostUStcp *hosta = new icon7::HostUStcp();
	hosta->SetRpcEnvironment(&rpc);
	hosta->Init();
	hosta->RunAsync();

	{
		icon7::commands::ExecuteBooleanOnHost com;
		com.function = [](auto host, bool v, void *) {
			printf(" %s\n", v ? "Listening" : "Fail to listen");
		};
		hosta->ListenOnPort(port, std::move(com), nullptr);
	}

	icon7::HostUStcp *hostb = new icon7::HostUStcp();
	hostb->SetRpcEnvironment(&rpc);
	hostb->Init();
	hostb->RunAsync();

	{
		icon7::commands::ExecuteOnPeer com;
		com.function = [](icon7::Peer *peer, auto x, void *) {
			DEBUG("Connected to host!");
			peer->Disconnect();
		};

		hostb->Connect("127.0.0.1", port, std::move(com), nullptr);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	auto f = hostb->ConnectPromise("127.0.0.1", port);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	std::thread t([&]() {
		DEBUG("Waiting to connect second!!!!!!!!!!!!!!!!!!!!!!");
		f.wait();
		icon7::Peer *peer = f.get();
		DEBUG("Connected to second!!!!!!!!!!!!!!!!!!!!!");

		rpc.Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);

		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		rpc.Call(peer, icon7::FLAG_RELIABLE,
				 icon7::OnReturnCallback::Make<uint32_t>(
					 [](icon7::Peer *peer, icon7::Flags flags,
						uint32_t result) -> void {
						 printf(" Multiplication result = %i\n", result);
					 },
					 [](icon7::Peer *peer) -> void {
						 printf(" Multiplication timeout\n");
					 },
					 10000, peer),
				 "mul", 5, 13);

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		delete hosta;
		delete hostb;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	printf(" CLOSING TIMEOUT!");
	DEBUG("Closing with timeout.");

	exit(0);

	icon7::Deinitialize();
	return 0;
}
