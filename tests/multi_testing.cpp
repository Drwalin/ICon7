#include <cstdio>

#include <chrono>
#include <thread>

#include "icon7/Command.hpp"
#include <icon7/Debug.hpp>
#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

#include "utility.hpp"

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

int main(int argc, char **argv)
{
	const ArgsParser args(argc, argv);
	
	icon7::Initialize();

	int notPassedTests = 0;
	const int testsCount = args.GetInt({"-tests", "-t"}, 1, 1, 10000);
	const int testsPerHostsPairCount = args.GetInt({"-pairs-per-test", "-p"}, 7, 1, 10000);
	const int sendsMoreThanCalls = args.GetInt({"-sends-per-send", "-ss"}, 117, 2, 100000);
	const int totalSends = args.GetInt({"-sends", "-s"}, 371, 1, 1000000);
	const int connectionsCount = args.GetInt({"-connections", "-con"}, 171, 1, 100000);
	int delayBetweeEachTotalSendMilliseconds = args.GetInt({"-delay", "-d"}, 50, 0, 10000);
	const int64_t maxWaitAfterPayloadDone =  args.GetInt({"-max-wait-after-payload"}, 3*60*1000, 0, 1000ll*3600ll*24ll*365ll);
	
	uint16_t port = args.GetInt({"-port"}, 7312, 1, 65535);
	
	bool printMoreStats = args.GetFlag({"-verbose-stats", "-stats"});
	
	const int serializeSendsModulo = args.GetInt({"-serialize-modulo"}, 5, 1, 1000000);
	const int serializeSendsFract = args.GetInt({"-serialize-load"}, 4, 1, 1000000);
	
	const bool alwaysReturn0 = args.GetFlag({"-return-zero"});

	int toReturnCount = 0;
	int totalToReturnCount = 0;

	std::vector<double> arrayOfLatency;
	arrayOfLatency.reserve(totalSends * connectionsCount);

	for (int i = 0; i < testsCount; ++i) {
		uint32_t totalReceived = 0, totalSent = 0, totalReturned = 0;
		LOG_INFO("Iteration: %i started", i);
		port++;
		if (port <= 1024) {
			port = 1025;
		}

		icon7::RPCEnvironment rpc;
		rpc.RegisterMessage("sum", Sum);
		rpc.RegisterMessage("mul", Mul);
		icon7::uS::tcp::Host *hosta = new icon7::uS::tcp::Host();
		hosta->SetRpcEnvironment(&rpc);
		hosta->Init();
		hosta->RunAsync();
		auto listenFuture = hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4);

		icon7::RPCEnvironment rpc2;
		rpc2.RegisterMessage("sum", Sum);
		rpc2.RegisterMessage("mul", Mul);
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

		class CommandPrintThreadId final : public icon7::Command
		{
		public:
			std::string name;
			CommandPrintThreadId(std::string name) : name(name) {}
			virtual ~CommandPrintThreadId() {}
			virtual void Execute() override
			{
				LOG_INFO("Thread %s", name.c_str());
			}
		};
		hosta->EnqueueCommand(
			icon7::CommandHandle<CommandPrintThreadId>::Create("Receiver"));
		hostb->EnqueueCommand(
			icon7::CommandHandle<CommandPrintThreadId>::Create("Sender"));

		const auto beginingBeforeConnectingInIteration =
			std::chrono::steady_clock::now();

		for (int pq = 0; pq < testsPerHostsPairCount; ++pq) {
			sent = received = returned = 0;
			for (auto &v : arrayOfLatency) {
				v = 0.0;
			}
			if (printMoreStats) {
				printf("\n");
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

				if (peer == nullptr) {
					LOG_WARN("Failed to etablish connection: %i of tested %i "
							 "of total %lu, due to nullptr value of future",
							 JJ, II, peers.size());
					++notPassedTests;
					continue;
				}

				for (int j = 0; j < 1000; ++j) {
					if (peer->IsReadyToUse() == false &&
						!peer->IsDisconnecting() && !peer->IsClosed() &&
						!peer->HadConnectError()) {
						if (j == 999) {
							LOG_WARN("Failed to etablish connection");
						}
						std::this_thread::sleep_for(
							std::chrono::milliseconds(1));
					} else {
						break;
					}
				}

				if (peer->HadConnectError() || peer->IsClosed()) {
					++JJ;
					LOG_WARN("Failed to etablish connection: %i of tested %i "
							 "of total %lu",
							 JJ, II, peers.size());
					++notPassedTests;
					continue;
				}

				if (peer->GetPeerStateFlags() != 1) {
					LOG_DEBUG("Peer state: %u", peer->GetPeerStateFlags());
				}

				validPeers.emplace_back(f.get());
			}

			LOG_INFO("Connected peers: %lu / %lu", validPeers.size(),
					 peers.size());

			peers.clear();

			toReturnCount = validPeers.size() * totalSends;

			uint32_t sendFrameSize = 0;
			const auto _S = std::chrono::steady_clock::now();
			double sumTim = 0;
			{
				auto commandsBuffer =
					hostb->GetCommandExecutionQueue()->GetThreadLocalBuffer();

				for (int k = 0; k < totalSends; ++k) {
					for (auto p : validPeers) {
						auto peer = p.get();
						auto curTim = std::chrono::steady_clock::now();
						auto onReturned = [curTim, &sumTim, &arrayOfLatency](
											  icon7::Peer *peer,
											  icon7::Flags flags,
											  uint32_t result) -> void {
							auto n = std::chrono::steady_clock::now();
							auto dt = std::chrono::duration<double>(n - curTim)
										  .count();
							sumTim += dt;
							uint32_t i = returned++;
							arrayOfLatency[i] = dt;
						};
						rpc2.Call(commandsBuffer, peer, icon7::FLAG_RELIABLE,
								  icon7::OnReturnCallback::Make<uint32_t>(
									  std::move(onReturned),
									  [](icon7::Peer *peer) -> void {
										  printf(" Multiplication timeout\n");
									  },
									  24 * 3600 * 1000, peer),
								  "mul", 5, 13);
						sent++;
					}
					commandsBuffer->FlushBuffer();

					icon7::ByteBuffer buffer(100);
					rpc2.SerializeSend(buffer, icon7::FLAG_RELIABLE, "sum", 3,
									   23);
					sendFrameSize = buffer.size();

					for (int l = 0; l < sendsMoreThanCalls - 1; ++l) {
						if (l % serializeSendsModulo < serializeSendsFract) {
							for (auto p : validPeers) {
								auto peer = p.get();
								peer->Send(buffer);
								sent++;
							}
						} else {
							for (auto p : validPeers) {
								auto peer = p.get();
								rpc2.Send(peer, icon7::FLAG_RELIABLE, "sum", 3,
										  23);
								sent++;
							}
						}
					}
					commandsBuffer->FlushBuffer();
					std::this_thread::sleep_for(std::chrono::milliseconds(
						delayBetweeEachTotalSendMilliseconds));
				}
			}

			auto _E = std::chrono::steady_clock::now();
			double _sec = std::chrono::duration<double>(_E - _S).count();
			double kops = (received.load() + returned.load()) / _sec / 1000.0;
			double mibps = kops * sendFrameSize * 1000.0 / 1024.0 / 1024.0;
			LOG_INFO("Instantenous throughput: %f Kops (%f MiBps)", kops,
					 mibps);

			_sec = std::chrono::duration<double>(
					   _E - beginingBeforeConnectingInIteration)
					   .count();
			kops = (totalReceived + totalReturned + received.load() +
					returned.load()) / _sec / 1000.0;
			mibps = kops * sendFrameSize * 1000.0 / 1024.0 / 1024.0;
			LOG_INFO("Total throughput: %f Kops (%f MiBps)", kops, mibps,
					 sumTim * 1000.0 / ((double)returned.load()));

			if (printMoreStats) {
				DataStats stats =
					CalcDataStats(arrayOfLatency.data(), returned.load());
				{
					double *vs = &stats.mean;
					for (int i = 0; i < sizeof(stats) / sizeof(double); ++i) {
						vs[i] *= 1000.0;
					}
				}
				LOG_INFO("Latency [ms] | avg: %.2f  std: %.2f    p0: %.2f   "
						 "p1: %.2f   p5: %.2f   p10: %.2f   p25: %.2f   p50: "
						 "%.2f   p80: %.2f   p90: %.2f   p95: %.2f   p99: %.2f "
						 "  p99.9: %.2f   p100: %.2f",
						 stats.mean, stats.stddev, stats.p0, stats.p1, stats.p5,
						 stats.p10, stats.p25, stats.p50, stats.p80, stats.p90,
						 stats.p95, stats.p99, stats.p999, stats.p100);
			}

			for (int64_t i = 0; i < maxWaitAfterPayloadDone; ++i) {
				if (sent == received && returned == toReturnCount) {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}

			for (auto &p : validPeers) {
				p->Disconnect();
			}
			hosta->DisconnectAllAsync();
			hostb->DisconnectAllAsync();
			hostb->WakeUp();

			for (auto &p : validPeers) {
				if (p->IsDisconnecting() == false || p->IsClosed() == false) {
					std::this_thread::sleep_for(std::chrono::microseconds(1));
				}
			}

			validPeers.clear();

			if (sent < 8) {
				notPassedTests++;
			}
			
			totalSent += sent;
			totalReceived += received;
			totalReturned += returned;
			totalToReturnCount += toReturnCount;
		}

		hosta->_InternalDestroy();
		hostb->_InternalDestroy();

		if (totalSent != totalReceived || totalReturned != totalToReturnCount ||
			notPassedTests) {
			LOG_INFO("Iteration: %i FAILED: received/sent = %i/%i ; "
					 "returned/called = %i/%i",
					 i, totalReceived, totalSent, totalReturned,
					 totalToReturnCount);
			notPassedTests++;
		} else {
			LOG_INFO("Iteration: %i finished: received/sent = %i/%i ; "
					 "returned/called = %i/%i",
					 i, totalReceived, totalSent, totalReturned,
					 totalToReturnCount);
		}

		delete hosta;
		delete hostb;
	}

	icon7::Deinitialize();

	if (notPassedTests) {
		LOG_DEBUG("Failed: %i/%i tests", notPassedTests, testsCount);
		if (alwaysReturn0 == false) {
			return 1;
		}
	} else {
		LOG_DEBUG("Success");
	}

	return 0;
}
