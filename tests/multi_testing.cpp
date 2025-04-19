#include <cstdio>

#include <thread>

#ifndef ICON7_USE_RPMALLOC
#define ICON7_USE_RPMALLOC 1
#endif

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Debug.hpp"
#include "../include/icon7/Peer.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Flags.hpp"
#include "../include/icon7/PeerUStcp.hpp"
#include "../include/icon7/HostUStcp.hpp"
#include "../include/icon7/LoopUS.hpp"

#if defined(__unix__) && ICON7_USE_RPMALLOC
#include <sstream>

#include "../include/icon7/Time.hpp"
#include "../include/icon7/MemoryPool.hpp"

#include <sys/syscall.h>
#include <unistd.h>
#endif

#include "utility.hpp"

#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_TRACE
#undef LOG_WARN

#define LOG_DEBUG(...) LOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) LOG_ERROR(__VA_ARGS__)
#define LOG_TRACE(...) LOG_ERROR(__VA_ARGS__)
#define LOG_WARN(...) LOG_ERROR(__VA_ARGS__)

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

#if defined(__unix__) && ICON7_USE_RPMALLOC
std::string ReadFile(std::string fileName)
{
	std::string ret;
	FILE *f = fopen(fileName.c_str(), "r");
	if (f) {
		int64_t offset = 0;
		while (true) {
			ret.resize(offset + 1024 * 64);
			int64_t size = fread(ret.data() + offset, 1, ret.size(), f);
			if (size > 0) {
				size += offset;
				offset = size;
				ret[size] = 0;
				ret.resize(size);
			} else {
				ret.resize(offset);
				break;
			}
		}
		fclose(f);
	}
	return ret;
}

std::stringstream ReadProcFile(int64_t pid, std::string metric)
{
	std::string data = ReadFile("/proc/" + std::to_string(pid) + "/" + metric);
	return std::stringstream(data);
}

struct ProcPidStatm {
	int64_t VmSize;
	int64_t VmRSS;
	int64_t shared;
	int64_t text;
	int64_t data;
};

ProcPidStatm proc_pid_statm(int64_t pid)
{
	ProcPidStatm ret;
	std::stringstream ss = ReadProcFile(pid, "statm");
	int64_t v;
	ss >> ret.VmSize >> ret.VmRSS >> ret.shared >> ret.text >> v >> ret.data >>
		v;
	return ret;
}

struct ProcPidStat {
	int64_t pid;
	std::string comm;
	char state;
	int64_t ppid;
	int64_t pgrp;
	int64_t session;
	int64_t tty_nr;
	int64_t tpgid;
	uint64_t flags;
	uint64_t minflt;
	uint64_t cminflt;
	uint64_t majflt;
	uint64_t cmajflt;
	uint64_t utim;
	uint64_t stime;
	int64_t cutim;
	int64_t cstime;
	int64_t priority;
	int64_t nice;
	int64_t num_threads;
	int64_t itrealvalue;
	uint64_t starttime;
	union {
		uint64_t vsize;
		uint64_t VmSize;
	};
	union {
		int64_t rss;
		int64_t VmRSS;
	};
	uint64_t rsslim;
	uint64_t startcode;
	uint64_t endcode;
	uint64_t startstack;
	uint64_t kstkesp;
	uint64_t kstkeip;
	uint64_t signal;
	uint64_t blocked;
	uint64_t sigignore;
	uint64_t sigcatch;
	uint64_t wchan;
	uint64_t nswap;
	uint64_t cnswap;
	int64_t exit_signal;
	int64_t processor;
	uint64_t rt_priority;
	uint64_t policy;
	uint64_t delayacct_blkio_ticks;
	uint64_t guest_time;
	int64_t cguest_time;
	uint64_t start_data;
	uint64_t end_data;
	uint64_t start_brk;
	uint64_t arg_start;
	uint64_t arg_end;
	uint64_t env_start;
	uint64_t env_end;
	int64_t exit_code;
};

ProcPidStat proc_pid_stat(int64_t pid)
{
	std::string s;
	ProcPidStat ret;
	std::stringstream ss = ReadProcFile(pid, "stat");
	ss >> ret.pid;

	std::getline(ss, ret.comm, '(');
	std::getline(ss, ret.comm, ')');

	printf("comm = `%s`\n", ret.comm.c_str());

	ss >> ret.state >> ret.ppid >> ret.pgrp >> ret.session >> ret.tty_nr >>
		ret.tpgid >> ret.flags >> ret.minflt >> ret.cminflt >> ret.majflt >>
		ret.cmajflt >> ret.utim >> ret.stime >> ret.cutim >> ret.cstime >>
		ret.priority >> ret.nice >> ret.num_threads >> ret.itrealvalue >>
		ret.starttime >> ret.vsize >> ret.rss >> ret.rsslim >> ret.startcode >>
		ret.endcode >> ret.startstack >> ret.kstkesp >> ret.kstkeip >>
		ret.signal >> ret.blocked >> ret.sigignore >> ret.sigcatch >>
		ret.wchan >> ret.nswap >> ret.cnswap >> ret.exit_signal >>
		ret.processor >> ret.rt_priority >> ret.policy >>
		ret.delayacct_blkio_ticks >> ret.guest_time >> ret.cguest_time >>
		ret.start_data >> ret.end_data >> ret.start_brk >> ret.arg_start >>
		ret.arg_end >> ret.env_start >> ret.env_end >> ret.exit_code;

	return ret;
}
#endif

void AsyncThreadMemoryAmountMetrics(std::string fileName)
{
#if defined(__unix__) && ICON7_USE_RPMALLOC
	std::thread([=]() {
		FILE *file = fopen(fileName.c_str(), "w");
		fprintf(file, "# time[s] - "
					  "totalAllocation - "
					  "totalAllocated[MiB] - "
					  "inUsePeak[MiB] - "
					  "currentlyInUse[MiB] - "
					  "VmPeak[MiB] - "
					  "VmSize[MiB] - "
					  "VmHWM[MiB] - "
					  "VmRSS[MiB]"
					  "\n");
		fflush(file);
		int64_t VmPeak = 0, VmHWM = 0;
		const icon7::time::Point start = icon7::time::GetTemporaryTimestamp();
		const icon7::time::Diff period = icon7::time::milliseconds(10);
		const int64_t procId = syscall(SYS_gettid);
		const int64_t pageSize = getpagesize();
		while (true) {
			const icon7::time::Point now = icon7::time::GetTemporaryTimestamp();
			const icon7::time::Diff delta = now - start;
			const double deltaSeconds =
				icon7::time::NanosecondsToSeconds(delta);

			const int64_t allocations =
				icon7::MemoryPool::stats.smallAllocations +
				icon7::MemoryPool::stats.mediumAllocations +
				icon7::MemoryPool::stats.largeAllocations;
			const int64_t totalAllocated =
				icon7::MemoryPool::stats.allocatedBytes;
			const int64_t currentlyInUse =
				(totalAllocated - icon7::MemoryPool::stats.deallocatedBytes);
			const int64_t inUsePeak = icon7::MemoryPool::stats.maxInUseAtOnce;

			ProcPidStatm statm = proc_pid_statm(procId);
			const int64_t VmSize = statm.VmSize;
			const int64_t VmRSS = statm.VmRSS;
			VmPeak = std::max(VmPeak, statm.VmSize);
			VmHWM = std::max(VmHWM, statm.VmRSS);

			fprintf(file, "%.4f %ld %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
					deltaSeconds, allocations,
					totalAllocated / (1024.0 * 1024.0),
					inUsePeak / (1024.0 * 1024.0),
					currentlyInUse / (1024.0 * 1024.0),
					VmPeak * pageSize / (1024.0 * 1024.0),
					VmSize * pageSize / (1024.0 * 1024.0),
					VmHWM * pageSize / (1024.0 * 1024.0),
					VmRSS * pageSize / (1024.0 * 1024.0));
			fflush(file);

			if (delta < period * 1000) {
				icon7::time::Sleep(delta / 1000);
			} else {
				icon7::time::Sleep(period);
			}
		}
		fclose(file);
	}).detach();
#endif
}

int main(int argc, char **argv)
{
	const ArgsParser args(argc, argv);

	icon7::Initialize();

	AsyncThreadMemoryAmountMetrics("memoryStats.mem");

	int notPassedTests = 0;
	const int testsCount = args.GetInt({"-tests", "-t"}, 1, 1, 1000000000);
	const int testsPerHostsPairCount =
		args.GetInt({"-pairs-per-test", "-p"}, 7, 1, 1000000000);
	const int sendsMoreThanCalls =
		args.GetInt({"-sends-per-send", "-ss"}, 117, 2, 1000000000);
	const int totalSends = args.GetInt({"-sends", "-s"}, 371, 1, 1000000000);
	const int additionalPayloadSize = args.GetInt(
		{"-additional-payload", "-payload"}, 1, 1, 256 * 1024 * 1024);
	const int connectionsCount =
		args.GetInt({"-connections", "-con"}, 171, 1, 1000000);
	icon7::time::Diff delayBetweeEachTotalSend = icon7::time::microseconds(
		args.GetInt({"-delay", "-d"}, 0, 0, 10000) * 500); // gets milliseconds
	const int64_t maxWaitAfterPayloadDone =
		args.GetInt({"-max-wait-after-payload"}, 1 * 60 * 1000, 0,
					1000ll * 3600ll * 24ll * 365ll);
	const int useSSL = args.GetFlag({"-ssl"});

	uint16_t port = args.GetInt({"-port"}, 7312, 1, 65535);

	bool printMoreStats = args.GetFlag({"-verbose-stats", "-stats"});

	const int serializeSendsModulo =
		args.GetInt({"-serialize-modulo"}, 5, 1, 1000000);
	const int serializeSendsFract =
		args.GetInt({"-serialize-load"}, 4, 1, 1000000);

	const bool alwaysReturn0 = args.GetFlag({"-return-zero"});

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

	for (int i = 0; i < testsCount; ++i) {
		uint64_t totalReceived = 0, totalSent = 0, totalReturned = 0;
		LOG_INFO("Iteration: %i started", i);
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

		listenFuture.wait_for_milliseconds(150);
		if (listenFuture.get() == false) {
			LOG_FATAL("Cannot listen on port: %i", (int)port);
			loopa->Destroy();
			loopb->Destroy();
			loopa = nullptr;
			loopb = nullptr;
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
			icon7::time::GetTemporaryTimestamp();

		for (int pq = 0; pq < testsPerHostsPairCount; ++pq) {
			sent = received = returned = 0;
			for (auto &v : arrayOfLatency) {
				v = 0.0;
			}
			if (printMoreStats) {
				printf("\n\n");
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
						icon7::time::Sleep(icon7::time::milliseconds(1));
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

			LOG_INFO("Hosts pair %i | failed %i | connected peers: %lu / %lu",
					 pq, notPassedTests, validPeers.size(), peers.size());

			peers.clear();

			toReturnCount = validPeers.size() * totalSends;

			uint32_t sendFrameSize = 0;
			const auto _S = icon7::time::GetTemporaryTimestamp();
			std::atomic<double> sumTim = 0;
			{
				icon7::CommandsBufferHandler commandsBuffer{
					hostb->GetCommandExecutionQueue()};

				for (int k = 0; k < totalSends; ++k) {
					if (k > 0) {
						icon7::time::Sleep(delayBetweeEachTotalSend);
					}
					for (auto p : validPeers) {
						auto peer = p.get();
						auto curTim = icon7::time::GetTemporaryTimestamp();
						auto onReturned = [curTim, &sumTim, &arrayOfLatency](
											  icon7::Peer *peer,
											  icon7::Flags flags,
											  uint32_t result) -> void {
							auto n = icon7::time::GetTemporaryTimestamp();
							auto dt =
								icon7::time::NanosecondsToSeconds(n - curTim);
							sumTim += dt;
							uint32_t i = returned++;
							arrayOfLatency[i] = dt;
						};
						rpc2.Call(&commandsBuffer, peer, icon7::FLAG_RELIABLE,
								  icon7::OnReturnCallback::Make<uint32_t>(
									  std::move(onReturned),
									  [](icon7::Peer *peer) -> void {
										  printf(" Multiplication timeout\n");
									  },
									  24 * 3600 * 1000, peer),
								  "mul", 5, 13, additionalPayload.data());
						sent++;
					}
					commandsBuffer.FlushBuffer();
					icon7::time::Sleep(delayBetweeEachTotalSend);

					for (int l = 0; l < sendsMoreThanCalls - 1; ++l) {
						if (l % serializeSendsModulo < serializeSendsFract) {
							icon7::ByteBuffer buffer(100);
							rpc2.SerializeSend(buffer, icon7::FLAG_RELIABLE,
											   "sum", 3, 23,
											   additionalPayload.data());
							sendFrameSize = buffer.size();
							for (auto p : validPeers) {
								auto peer = p.get();
								peer->Send(buffer);
								sent++;
							}
						} else {
							for (auto p : validPeers) {
								auto peer = p.get();
								rpc2.Send(peer, icon7::FLAG_RELIABLE, "sum", 3,
										  23, additionalPayload.data());
								sent++;
							}
						}
					}
					commandsBuffer.FlushBuffer();
				}
			}

			const auto _E = icon7::time::GetTemporaryTimestamp();
			double _sec = icon7::time::NanosecondsToSeconds(_E - _S);
			double kops = (received.load() + returned.load()) / _sec / 1000.0;
			double mibps = kops * sendFrameSize * 1000.0 / 1024.0 / 1024.0;
			LOG_INFO("Instantenous throughput: %f Kops (%f MiBps)", kops,
					 mibps);

			_sec = icon7::time::NanosecondsToSeconds(
				_E - beginingBeforeConnectingInIteration);
			kops = (totalReceived + totalReturned + received.load() +
					returned.load()) /
				   _sec / 1000.0;
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
				LOG_INFO("received/sent = %li/%li ; "
						 "returned/called = %li/%li",
						 received.load(), sent.load(), returned.load(),
						 toReturnCount);
				LOG_INFO("Waiting to finish message transmission");
			}

			uint32_t oldReceived = received.load(),
					 oldReturned = returned.load();
			auto startWaitTimepoint = icon7::time::GetTemporaryTimestamp();
			for (int64_t i = 0; i < maxWaitAfterPayloadDone; ++i) {
				if (received >= sent && returned >= toReturnCount) {
					break;
				}
				if (oldReceived != received.load() ||
					oldReturned != returned.load()) {
					oldReceived = received.load();
					oldReturned = returned.load();
					startWaitTimepoint = icon7::time::GetTemporaryTimestamp();
				} else {
					const auto now = icon7::time::GetTemporaryTimestamp();
					if ((now - startWaitTimepoint) > icon7::time::seconds(5)) {
						notPassedTests++;
						break;
					}
				}
				icon7::time::Sleep(icon7::time::microseconds(950));
			}

			if (printMoreStats) {
				LOG_INFO("received/sent = %li/%li ; "
						 "returned/called = %li/%li",
						 received.load(), sent.load(), returned.load(),
						 toReturnCount);
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
		}

		loopa->Destroy();
		loopb->Destroy();

		if (totalSent != totalReceived || totalReturned != totalToReturnCount ||
			notPassedTests) {
			LOG_INFO("Iteration: %i FAILED: received/sent = %li/%li ; "
					 "returned/called = %li/%li",
					 i, totalReceived, totalSent, totalReturned,
					 totalToReturnCount);
			notPassedTests++;
		} else {
			LOG_INFO("Iteration: %i finished: received/sent = %li/%li ; "
					 "returned/called = %li/%li",
					 i, totalReceived, totalSent, totalReturned,
					 totalToReturnCount);
		}

		loopa = nullptr;
		loopb = nullptr;
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
