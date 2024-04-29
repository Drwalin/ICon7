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
	const int sendsMoreThanCalls = 37;
	const int totalSends = 23;
	const int connectionsCount = 31;
	const int testsCount = 13;

	for (int i = 0; i < testsCount; ++i) {
		sent = received = returned = 0;
		LOG_INFO("Iteration: %i started", i);
		icon7::RPCEnvironment rpc;
		rpc.RegisterMessage("sum", Sum);
		rpc.RegisterMessage("mul", Mul);
		port++;

		icon7::uS::tcp::Host *hosta = new icon7::uS::tcp::Host();
		hosta->SetRpcEnvironment(&rpc);
		hosta->Init();
		hosta->RunAsync();
		auto listenFuture = hosta->ListenOnPort("127.0.0.1", port, icon7::IPv4);

		icon7::uS::tcp::Host *hostb = new icon7::uS::tcp::Host();
		hostb->SetRpcEnvironment(&rpc);
		hostb->Init();
		hostb->RunAsync();

		listenFuture.wait_for(std::chrono::milliseconds(150));
		
		std::vector<concurrent::future<std::shared_ptr<icon7::Peer>>> peers;
		for (int i = 0; i < connectionsCount; ++i) {
			peers.push_back(hostb->ConnectPromise("127.0.0.1", port));
		}
		for (auto &f : peers) {
			f.wait();
		}
		for (auto &f : peers) {
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
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				LOG_WARN("Failed to etablish connection");
				continue;
			}

			for (int k=0; k<totalSends; ++k) {
				for (int l=0; l<sendsMoreThanCalls-1; ++l) {
					rpc.Send(peer, icon7::FLAG_RELIABLE, "sum", 3, 23);
					sent++;
				}

				rpc.Call(peer, icon7::FLAG_RELIABLE,
						 icon7::OnReturnCallback::Make<uint32_t>(
							 [](icon7::Peer *peer, icon7::Flags flags,
								uint32_t result) -> void { returned++; },
							 [](icon7::Peer *peer) -> void {
								 printf(" Multiplication timeout\n");
							 },
							 1000000, peer),
						 "mul", 5, 13);
				sent++;
			}

			if (peer->GetPeerStateFlags() != 1) {
				LOG_DEBUG("Peer state: %u", peer->GetPeerStateFlags());
			}
		}

		peers.clear();

		for (int i = 0; i < 1000; ++i) {
			if (sent == received && returned == sent / sendsMoreThanCalls) {
				break;
			}
			hosta->WakeUp();
			hostb->WakeUp();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		hosta->_InternalDestroy();
		hostb->_InternalDestroy();

		if (sent < 8) {
			notPassedTests++;
		}

		if (sent != received || returned != sent / sendsMoreThanCalls || notPassedTests) {
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

		{
		LOG_DEBUG("master pool before: malloc: %lu  free: %lu  objResid: %lu  memResid: %lu  bucketAcq: %lu  bucketRel: %lu  objectAcq: %lu  objectRel: %lu  objInGlob: %lu  objBucAcq: %lu  objBucRel: %lu",
				icon7::globalPool.estimate_system_allocations(),
				icon7::globalPool.estimate_system_frees(),
				icon7::globalPool.current_memory_resident_objects(),
				icon7::globalPool.current_memory_resident(),
				icon7::globalPool.count_bucket_acquisitions(),
				icon7::globalPool.count_bucket_releases(),
				icon7::globalPool.local_sum_acquisition.load(),
				icon7::globalPool.local_sum_release.load(),
				icon7::globalPool.count_objects_in_global_pool(),
				icon7::globalPool.sum_object_acquisition.load(),
				icon7::globalPool.sum_object_release.load()
				);
			auto &s =
	icon7::globalPool.mod_tls_pool<128>(nullptr, false);
	for (auto it : s) {
		it->release_buckets_to_global();
		LOG_DEBUG("master pool releas: malloc: %lu  free: %lu  objResid: %lu  memResid: %lu  bucketAcq: %lu  bucketRel: %lu  objectAcq: %lu  objectRel: %lu  objInGlob: %lu  objBucAcq: %lu  objBucRel: %lu",
				icon7::globalPool.estimate_system_allocations(),
				icon7::globalPool.estimate_system_frees(),
				icon7::globalPool.current_memory_resident_objects(),
				icon7::globalPool.current_memory_resident(),
				icon7::globalPool.count_bucket_acquisitions(),
				icon7::globalPool.count_bucket_releases(),
				icon7::globalPool.local_sum_acquisition.load(),
				icon7::globalPool.local_sum_release.load(),
				icon7::globalPool.count_objects_in_global_pool(),
				icon7::globalPool.sum_object_acquisition.load(),
				icon7::globalPool.sum_object_release.load()
				);
			}
		}
	
		icon7::globalPool.free_all();
		LOG_DEBUG("master pool  after: malloc: %lu  free: %lu  objResid: %lu  memResid: %lu  bucketAcq: %lu  bucketRel: %lu  objectAcq: %lu  objectRel: %lu  objInGlob: %lu  objBucAcq: %lu  objBucRel: %lu",
				icon7::globalPool.estimate_system_allocations(),
				icon7::globalPool.estimate_system_frees(),
				icon7::globalPool.current_memory_resident_objects(),
				icon7::globalPool.current_memory_resident(),
				icon7::globalPool.count_bucket_acquisitions(),
				icon7::globalPool.count_bucket_releases(),
				icon7::globalPool.local_sum_acquisition.load(),
				icon7::globalPool.local_sum_release.load(),
				icon7::globalPool.count_objects_in_global_pool(),
				icon7::globalPool.sum_object_acquisition.load(),
				icon7::globalPool.sum_object_release.load()
				);

	if (notPassedTests) {
		LOG_DEBUG("Failed: %i/%i tests", notPassedTests, testsCount);
		return -1;
	} else {
		LOG_DEBUG("Success");
	}

	return 0;
}
