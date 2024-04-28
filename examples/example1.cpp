#include "icon7/Command.hpp"
#include <cstdio>

#include <chrono>
#include <thread>

#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

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

	icon7::RPCEnvironment rpc;
	rpc.RegisterMessage("sum", Sum);
	rpc.RegisterMessage("mul", Mul);

	icon7::uS::tcp::Host *hosta = new icon7::uS::tcp::Host();
	hosta->SetRpcEnvironment(&rpc);
	hosta->Init();
	hosta->RunAsync();

	{
		class CommandPrintStatus : public icon7::commands::ExecuteBooleanOnHost
		{
		public:
			CommandPrintStatus() = default;
			~CommandPrintStatus() = default;
			virtual void Execute() override
			{
				printf(" %s\n", result ? "Listening" : "Fail to listen");
			}
		};
		auto com = icon7::CommandHandle<CommandPrintStatus>::Create();
		hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4, std::move(com),
							nullptr);
	}

	icon7::uS::tcp::Host *hostb = new icon7::uS::tcp::Host();
	hostb->SetRpcEnvironment(&rpc);
	hostb->Init();
	hostb->RunAsync();

	{
		class CommandPrintStatus : public icon7::commands::ExecuteOnPeer
		{
		public:
			CommandPrintStatus() = default;
			~CommandPrintStatus() = default;
			virtual void Execute() override { peer->Disconnect(); }
		};
		auto com = icon7::CommandHandle<CommandPrintStatus>::Create();
		hostb->Connect("127.0.0.1", port, std::move(com), nullptr);
	}

	std::thread t(
		[](icon7::RPCEnvironment *rpc, auto hostb, auto port) {
			auto f = hostb->ConnectPromise("127.0.0.1", port);
			f.wait();
			auto _peer = f.get();
			icon7::Peer *peer = _peer.get();

			rpc->Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);

			std::this_thread::sleep_for(std::chrono::milliseconds(5));

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
		&rpc, hostb, port);

	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	hosta->_InternalDestroy();
	hostb->_InternalDestroy();

	delete hosta;
	delete hostb;
	t.join();

	icon7::Deinitialize();
	return 0;
}
