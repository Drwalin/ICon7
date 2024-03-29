#include "icon7/Command.hpp"
#include <cstdio>

#include <future>
#include <iostream>
#include <chrono>
#include <thread>

#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

int Sum(int a, int b) { return a + b; }

int Mul(int a, int b) { return a * b; }

int main()
{
	uint16_t port = 7312;

	icon7::Initialize();

	for (int i = 0; i < 10; ++i) {
		DEBUG("Iteration: %i", i);
		icon7::RPCEnvironment rpc;
		rpc.RegisterMessage("sum", Sum);
		rpc.RegisterMessage("mul", Mul);
		port++;

		icon7::uS::tcp::Host *hosta = new icon7::uS::tcp::Host();
		hosta->SetRpcEnvironment(&rpc);
		hosta->Init();
		hosta->RunAsync();

		{
			icon7::commands::ExecuteBooleanOnHost com;
			com.function = [](auto host, bool v, void *) {};
			hosta->ListenOnPort(port, icon7::IPv4, std::move(com), nullptr);
		}

		icon7::uS::tcp::Host *hostb = new icon7::uS::tcp::Host();
		hostb->SetRpcEnvironment(&rpc);
		hostb->Init();
		hostb->RunAsync();

		std::vector<std::shared_future<std::shared_ptr<icon7::Peer>>> peers;
		for (int i = 0; i < 10; ++i) {
			peers.push_back(hostb->ConnectPromise("127.0.0.1", port).share());
		}
		for (auto &f : peers) {
			f.wait();
			icon7::Peer *peer = f.get().get();

			rpc.Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);

			rpc.Call(peer, icon7::FLAG_RELIABLE,
					 icon7::OnReturnCallback::Make<uint32_t>(
						 [](icon7::Peer *peer, icon7::Flags flags,
							uint32_t result) -> void {},
						 [](icon7::Peer *peer) -> void {
							 printf(" Multiplication timeout\n");
						 },
						 10000, peer),
					 "mul", 5, 13);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		hosta->_InternalDestroy();
		hostb->_InternalDestroy();

		delete hosta;
		delete hostb;
	}

	icon7::Deinitialize();
	return 0;
}
