#include <cstdio>

#include <chrono>
#include <thread>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/PeerUStcp.hpp"
#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/LoopUS.hpp"

std::atomic<int64_t> sent = 0, received = 0, returned = 0;

int Sum(int a, int b, std::string str)
{
	received++;
	return a + b + strlen(str.c_str());
}

int Mul(int a, int b, std::string str)
{
	received++;
	return a * b + strlen(str.c_str());
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 64)
		return 0;

	icon7::Initialize();

	int notPassedTests = 0;
	const int sendsMoreThanCalls = 5;
	const int totalSends = 10;
	const int additionalPayloadSize = data[0] + 1;
	const int connectionsCount = 17;
	int delayBetweeEachTotalSendMilliseconds = 0;
	const int64_t maxWaitAfterPayloadDone = 1000;
	const int useSSL = data[1];

	uint16_t port = 7312;

	const int serializeSendsModulo = data[2] + 1;
	const int serializeSendsFract = data[3] + 1;

	int64_t toReturnCount = 0;
	int64_t totalToReturnCount = 0;

	std::vector<double> arrayOfLatency;
	arrayOfLatency.reserve(totalSends * connectionsCount);

	std::vector<char> additionalPayload;
	additionalPayload.resize(additionalPayloadSize);
	for (int i = 0; i < additionalPayload.size(); ++i) {
		static const char chars[] =
			"1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./"
			"`~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>? ";
		additionalPayload[i] = chars[((uint32_t)rand()) % (sizeof(chars) - 1)];
	}
	additionalPayload.back() = 0;

	while (true) {
		uint64_t totalReceived = 0, totalSent = 0, totalReturned = 0;
		port++;
		if (port <= 1024) {
			port = 1025;
		}

		icon7::RPCEnvironment rpc;
		rpc.RegisterMessage("sum", Sum);
		rpc.RegisterMessage("mul", Mul);
		std::shared_ptr<icon7::uS::Loop> loopa =
			std::make_shared<icon7::uS::Loop>();
		loopa->Init(1);
		std::shared_ptr<icon7::uS::tcp::Host> hosta =
			loopa->CreateHost(useSSL, "../cert/user.key", "../cert/user.crt",
							  "", nullptr, "../cert/rootca.crt",
							  "ECDHE-ECDSA-AES256-GCM-SHA384:"
							  "ECDHE-ECDSA-AES128-GCM-SHA256:"
							  "ECDHE-ECDSA-CHACHA20-POLY1305:"
							  "DHE-RSA-AES256-GCM-SHA384:");
		hosta->SetRpcEnvironment(&rpc);
		loopa->RunAsync();
		auto listenFuture = hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4);

		icon7::RPCEnvironment rpc2;
		rpc2.RegisterMessage("sum", Sum);
		rpc2.RegisterMessage("mul", Mul);
		std::shared_ptr<icon7::uS::Loop> loopb =
			std::make_shared<icon7::uS::Loop>();
		loopb->Init(1);
		std::shared_ptr<icon7::uS::tcp::Host> hostb = loopb->CreateHost(
			useSSL, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		hostb->SetRpcEnvironment(&rpc2);
		loopb->RunAsync();

		listenFuture.wait_for(std::chrono::milliseconds(150));
		if (listenFuture.get() == false) {
			loopa->Destroy();
			loopb->Destroy();
			loopa = nullptr;
			loopb = nullptr;
			continue;
		}

		sent = received = returned = 0;
		for (auto &v : arrayOfLatency) {
			v = 0.0;
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

		peers.clear();

		toReturnCount = validPeers.size() * totalSends;

		std::atomic<double> sumTim = 0;
		{
			auto commandsBuffer =
				hostb->GetCommandExecutionQueue()->GetThreadLocalBuffer();

			for (int k = 0; k < totalSends; ++k) {
				if (k > 0) {
					std::this_thread::sleep_for(std::chrono::microseconds(
						delayBetweeEachTotalSendMilliseconds * 500ll));
				}
				for (auto p : validPeers) {
					auto peer = p.get();
					auto curTim = std::chrono::steady_clock::now();
					auto onReturned = [curTim, &sumTim, &arrayOfLatency](
										  icon7::Peer *peer, icon7::Flags flags,
										  uint32_t result) -> void {
						auto n = std::chrono::steady_clock::now();
						auto dt =
							std::chrono::duration<double>(n - curTim).count();
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
							  "mul", 5, 13, additionalPayload.data());
					sent++;
				}
				commandsBuffer->FlushBuffer();
				std::this_thread::sleep_for(std::chrono::microseconds(
					delayBetweeEachTotalSendMilliseconds * 500ll));

				for (int l = 0; l < sendsMoreThanCalls - 1; ++l) {
					if (l % serializeSendsModulo < serializeSendsFract) {
						icon7::ByteBuffer buffer(100);
						rpc2.SerializeSend(buffer, icon7::FLAG_RELIABLE, "sum",
										   3, 23, additionalPayload.data());
						for (auto p : validPeers) {
							auto peer = p.get();
							peer->Send(buffer);
							sent++;
						}
					} else {
						for (auto p : validPeers) {
							auto peer = p.get();
							rpc2.Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23,
									  additionalPayload.data());
							sent++;
						}
					}
				}
				commandsBuffer->FlushBuffer();
			}
		}

		uint32_t oldReceived = received.load(), oldReturned = returned.load();
		auto startWaitTimepoint = std::chrono::steady_clock::now();
		for (int64_t i = 0; i < maxWaitAfterPayloadDone * 100; ++i) {
			if (received >= sent && returned >= toReturnCount) {
				break;
			}
			if (oldReceived != received.load() ||
				oldReturned != returned.load()) {
				const auto now = std::chrono::steady_clock::now();
				if ((now - startWaitTimepoint) > std::chrono::seconds(10)) {
					notPassedTests++;
					break;
				}
				startWaitTimepoint = now;
				oldReceived = received.load();
				oldReturned = returned.load();
			}
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		for (auto &p : validPeers) {
			p->Disconnect();
		}

		validPeers.clear();

		if (sent < 8) {
			notPassedTests++;
		}

		totalSent += sent;
		totalReceived += received;
		totalReturned += returned;
		totalToReturnCount += toReturnCount;

		loopa->Destroy();
		loopb->Destroy();

		if (totalSent != totalReceived || totalReturned != totalToReturnCount ||
			notPassedTests) {
			notPassedTests++;
		}

		loopa = nullptr;
		loopb = nullptr;
		break;
	}

	icon7::Deinitialize();
	return 0;
}
