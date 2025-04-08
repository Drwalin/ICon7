#include "icon7/Command.hpp"
#include <cstdio>

#include <thread>

#include <icon7/Time.hpp>
#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>
#include "../include/icon7/LoopUS.hpp"

int Sum(int a, int b)
{
	printf(" %i + %i = %i\n", a, b, a + b);
	return a + b;
}

int Mul(int a, int b)
{
	printf(" %i * %i = %i\n", a, b, a * b);
	return a * b;
}

int main()
{
	uint16_t port = 7312;

	icon7::Initialize();

	icon7::RPCEnvironment rpc, rpc2;
	rpc.RegisterMessage("sum", Sum);
	rpc.RegisterMessage("mul", Mul);
	rpc2.RegisterMessage("sum", Sum);
	rpc2.RegisterMessage("mul", Mul);

	std::shared_ptr<icon7::uS::Loop> loopa =
		std::make_shared<icon7::uS::Loop>();
	loopa->Init(1);
	std::shared_ptr<icon7::uS::tcp::Host> hosta = loopa->CreateHost(false);
	hosta->SetRpcEnvironment(&rpc);
	loopa->RunAsync();

	{
		auto f = hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4);
		f.wait_for_milliseconds(150);
		if (f.finished()) {
			printf("Listening\n");
		} else {
			printf("Fail to listen\n");
			return 0;
		}
	}

	std::shared_ptr<icon7::uS::Loop> loopb =
		std::make_shared<icon7::uS::Loop>();
	loopb->Init(1);
	std::shared_ptr<icon7::uS::tcp::Host> hostb = loopb->CreateHost(false);
	hostb->SetRpcEnvironment(&rpc2);
	loopb->RunAsync();

	{
		class CommandDisconnectOnConnect final
			: public icon7::commands::ExecuteOnPeer
		{
		public:
			CommandDisconnectOnConnect() {}
			virtual ~CommandDisconnectOnConnect() {}
			virtual void Execute() override { peer->Disconnect(); }
		};
		auto com = icon7::CommandHandle<CommandDisconnectOnConnect>::Create();
		hostb->Connect("127.0.0.1", port, std::move(com), nullptr);
	}

	std::thread t(
		[](icon7::RPCEnvironment *rpc, auto hostb, auto port) {
			auto f = hostb->ConnectPromise("127.0.0.1", port);
			f.wait();
			auto _peer = f.get();
			icon7::Peer *peer = _peer.get();

			rpc->Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);

			icon7::time::SleepMSec(5);

			rpc->Call(peer, icon7::FLAG_RELIABLE,
					  icon7::OnReturnCallback::Make<uint32_t>(
						  [](icon7::Peer *peer, icon7::Flags flags,
							 uint32_t result) -> void {
							  printf(" Multiplication result returned = %i\n",
									 result);
						  },
						  [](icon7::Peer *peer) -> void {
							  printf(" Multiplication timeout\n");
						  },
						  10000, peer),
					  "mul", 5, 13);
		},
		&rpc2, hostb, port);

	icon7::time::SleepMSec(300);

	hosta = nullptr;
	hostb = nullptr;
	loopa->Destroy();
	loopb->Destroy();

	t.join();

	icon7::Deinitialize();
	return 0;
}
