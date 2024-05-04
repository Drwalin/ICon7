#include "icon7/Command.hpp"
#include <cstdio>

#include <chrono>
#include <thread>

#include <icon7/Debug.hpp>
#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

std::atomic<int> sent = 0, received = 0, returned = 0;

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

int main()
{
	uint16_t port = 7312;

	icon7::Initialize();

	int notPassedTests = 0;
	const int sendsMoreThanCalls = 117;
	const int totalSends = 371;
	const int connectionsCount = 171;
	const int testsCount = 7;

	for (int i = 0; i < testsCount; ++i) {
		sent = received = returned = 0;
		LOG_INFO("Iteration: %i started", i);
		icon7::RPCEnvironment rpc, rpc2;
		rpc.RegisterMessage("sum", Sum);
		rpc.RegisterMessage("mul", Mul);
		rpc2.RegisterMessage("sum", Sum);
		rpc2.RegisterMessage("mul", Mul);
		port++;
		if (port <= 1024) {
			port = 1025;
		}

		icon7::uS::tcp::Host *hosta = new icon7::uS::tcp::Host();
		hosta->SetRpcEnvironment(&rpc);
		hosta->Init();
		hosta->RunAsync();
		auto listenFuture = hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4);

		icon7::uS::tcp::Host *hostb = new icon7::uS::tcp::Host();
		hostb->SetRpcEnvironment(&rpc2);
		hostb->Init();
		hostb->RunAsync();

		listenFuture.wait_for(std::chrono::milliseconds(150));
		if (listenFuture.get() == false) {
			LOG_FATAL("Cannot listen on port: %i", (int)port);
			hosta->_InternalDestroy();
			hostb->_InternalDestroy();
			delete hosta;
			delete hostb;
			--i;
			continue;
		}
		
		{
			class CommandPrintThreadId final : public icon7::Command
			{
			public:
				std::string name;
				CommandPrintThreadId(std::string name) : name(name) {
				}
				virtual ~CommandPrintThreadId() = default;
				virtual void Execute() override
				{
					LOG_INFO("Thread %s", name.c_str());
				}
			};
			hosta->EnqueueCommand(icon7::CommandHandle<CommandPrintThreadId>::Create("Receiver"));
			hostb->EnqueueCommand(icon7::CommandHandle<CommandPrintThreadId>::Create("Sender"));
		}

		std::vector<concurrent::future<std::shared_ptr<icon7::Peer>>> peers;
		for (int i = 0; i < connectionsCount; ++i) {
			peers.push_back(hostb->ConnectPromise("127.0.0.1", port));
		}
		int II = 0, JJ = 0;
		for (auto &f : peers) {
			++II;
			f.wait();
		}

		std::vector<std::shared_ptr<icon7::Peer>> validPeers;
		II = 0;
		for (auto &f : peers) {
			++II;
			f.wait();
			icon7::Peer *peer = f.get().get();

			for (int j = 0; j < 1000; ++j) {
				if (peer->IsReadyToUse() == false && !peer->IsDisconnecting() &&
					!peer->IsClosed() && !peer->HadConnectError()) {
					if (j == 999) {
						LOG_WARN("Failed to etablish connection");
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				} else {
					break;
				}
			}

			if (peer->HadConnectError() || peer->IsClosed()) {
				++JJ;
				LOG_WARN("Failed to etablish connection: %i of tested %i of toteal %lu", JJ, II, peers.size());
				++notPassedTests;
				continue;
			}

			if (peer->GetPeerStateFlags() != 1) {
				LOG_DEBUG("Peer state: %u", peer->GetPeerStateFlags());
			}

			validPeers.emplace_back(f.get());
		}
		
		LOG_INFO("Connected peers: %lu / %lu", validPeers.size(), peers.size());
		
		peers.clear();
		
		const auto _S = std::chrono::steady_clock::now();
		double sumTim = 0;
		{
			auto commandsBuffer =
				hostb->GetCommandExecutionQueue()->GetThreadLocalBuffer();
			for (int k = 0; k < totalSends; ++k) {
				for (auto p : validPeers) {
					auto peer = p.get();
					auto curTim = std::chrono::steady_clock::now();
					rpc2.Call(commandsBuffer, peer, icon7::FLAG_RELIABLE,
							  icon7::OnReturnCallback::Make<uint32_t>(
								  [curTim, &sumTim](icon7::Peer *peer, icon7::Flags flags,
									 uint32_t result) -> void { returned++;
									auto n = std::chrono::steady_clock::now();
								  	auto dt = std::chrono::duration<double>(n-curTim).count();
									sumTim += dt;
								  },
								  [](icon7::Peer *peer) -> void {
									  printf(" Multiplication timeout\n");
								  },
								  1000000, peer),
							  "mul", 5, 13);
					sent++;
				}
				commandsBuffer->FlushBuffer();
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				for (int l = 0; l < sendsMoreThanCalls - 1; ++l) {
					for (auto p : validPeers) {
						auto peer = p.get();
						rpc2.Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);
						sent++;
					}
				}
				commandsBuffer->FlushBuffer();
// 				std::this_thread::sleep_for(std::chrono::milliseconds(50));
// 				const auto _E = std::chrono::steady_clock::now();
// 				const double _sec = std::chrono::duration<double>(_E-_S).count();
// 				std::this_thread::sleep_for(std::chrono::milliseconds(72));
// 				const double kops = (received.load() + returned.load() ) / _sec / 1000.0;
// 				const double mibps = kops * 13 / 1000.0;
// 				LOG_TRACE("progress: %i/%i   Throughput: %f Kops (%f MiBps)   time: %.2f s", k, totalSends, kops, mibps, _sec);
			}
		}
		const auto _E = std::chrono::steady_clock::now();
		const double _sec = std::chrono::duration<double>(_E-_S).count();
		const double kops = (received.load() + returned.load() ) / _sec / 1000.0;
		const double mibps = kops * 13 / 1000.0;
		LOG_INFO("Throughput: %f Kops (%f MiBps),  avg latency: %f ms", kops, mibps, sumTim*1000.0/((double)returned.load()));

		validPeers.clear();

		for (int i = 0; i < 30000; ++i) {
			if (sent == received && returned == sent / sendsMoreThanCalls) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		hosta->_InternalDestroy();
		hostb->_InternalDestroy();

		if (sent < 8) {
			notPassedTests++;
		}

		if (sent != received || returned != sent / sendsMoreThanCalls ||
			notPassedTests) {
			LOG_INFO("Iteration: %i FAILED: sent/received = %i/%i ; "
					 "returned/called = %i/%i",
					 i, sent.load(), received.load(), returned.load(),
					 sent.load() / sendsMoreThanCalls);
			notPassedTests++;
		} else {
			LOG_INFO("Iteration: %i finished: sent/received = %i/%i ; "
					 "returned/called = %i/%i",
					 i, sent.load(), received.load(), returned.load(),
					 sent.load() / sendsMoreThanCalls);
		}

		delete hosta;
		delete hostb;
	}

	icon7::Deinitialize();

	if (notPassedTests) {
		LOG_DEBUG("Failed: %i/%i tests", notPassedTests, testsCount);
		return -1;
	} else {
		LOG_DEBUG("Success");
	}

	return 0;
}
