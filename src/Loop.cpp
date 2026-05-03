// Copyright (C) 2025-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <thread>
#include <string>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/RPCEnvironment.hpp"
#include "../include/icon7/Host.hpp"
#include "../include/icon7/Time.hpp"

#include "../include/icon7/Loop.hpp"

namespace icon7
{
Loop::Loop(std::string objectName) : peerManager(this), objectName(objectName)
{
	userData = 0;
	userPointer = nullptr;
	asyncRunnerFlags = 0;
}

Loop::~Loop() {
	LOG_INFO("Loop (%s) iteration: iterations: small: %lu   big: %lu   wakeups: %lu   timer wakeups: %lu"
			 "      errors: %lu"
			 "      bytes: send: %lu   recv: %lu",
			objectName.c_str(),
			stats.loopSmallIterations,
			stats.loopBigIterations,
			stats.loopWakeups,
			stats.loopTimerWakeups,
			stats.errorsCount,
			stats.bytesReceived,
			stats.bytesSent
			);
	WaitStopRunning();
}

PeerData *Loop::GetLocalPeerData(PeerHandle handle)
{
	// TODO: this is temporary, may lead to race condition
	return peerManager.GetLocalPeerData(handle);
}
std::shared_ptr<Peer> Loop::GetSharedPeer(PeerHandle handle)
{
	// TODO: this is temporary, may lead to race condition
	return peerManager.GetSharedPeer(handle);
}
Host *Loop::GetSharedHost(PeerHandle handle)
{
	// TODO: this is temporary, may lead to race condition
	return peerManager.GetSharedHost(handle).get();
}

void Loop::WaitStopRunning()
{
	QueueStopRunning();
	while (IsRunningAsync()) {
		QueueStopRunning();
		WakeUp();

		icon7::time::Sleep(icon7::time::microseconds(100));
	}
}

void Loop::Destroy()
{
	class CommandDestroyHosts final : public Command
	{
	public:
		CommandDestroyHosts() {}
		virtual ~CommandDestroyHosts() {}
		Loop *loop;
		virtual void Execute() override
		{
			loop->commandQueue.Execute(128);
			while (loop->commandQueue.HasAny()) {
				loop->commandQueue.Execute(128);
			}
			while (loop->hosts.empty() == false) {
				std::shared_ptr<icon7::Host> h = *loop->hosts.begin();
				h->_InternalDestroy();
			}
		}
	};
	auto com = CommandHandle<CommandDestroyHosts>::Create();
	com->loop = this;
	EnqueueCommand(std::move(com));

	WaitStopRunning();

	commandQueue.Execute(128);
	while (commandQueue.HasAny()) {
		commandQueue.Execute(128);
	}
}

void Loop::EnqueueCommand(CommandHandle<Command> &&command)
{
	commandQueue.EnqueueCommand(std::move(command));
	WakeUp();
}

CommandExecutionQueue::CoroutineAwaitable
Loop::Schedule(std::shared_ptr<void> &&obj)
{
	return commandQueue.Schedule(std::move(obj));
}

CommandExecutionQueue::CoroutineAwaitable Loop::Schedule()
{
	return commandQueue.Schedule();
}

void Loop::RunAsync()
{
	asyncRunnerFlags = 0;
	asyncRunner =
		std::thread(+[](Loop *loop) { loop->_InternalSyncLoop(); }, this);
	asyncRunner.detach();
}

void Loop::SingleLoopIteration() { stats.loopBigIterations += 1; }

void Loop::_InternalSyncLoop()
{
	asyncRunnerFlags |= RUNNING;
	while (IsQueuedStopAsync() == false) {
		SingleLoopIteration();
		if (microsecondsOfSleepBetweenIterations > 0) {
			icon7::time::Sleep(icon7::time::microseconds(
				microsecondsOfSleepBetweenIterations));
		}
	}
	asyncRunnerFlags &= ~RUNNING;
}

void Loop::QueueStopRunning()
{
	asyncRunnerFlags |= QUEUE_STOP;
	if (IsRunningAsync()) {
		WakeUp();
	}
}

bool Loop::IsQueuedStopAsync() { return asyncRunnerFlags & QUEUE_STOP; }

bool Loop::IsRunningAsync() { return asyncRunnerFlags & RUNNING; }

void Loop::_InternalSingleLoopIteration()
{
	stats.loopSmallIterations += 1;
// 	for (int i=0; i<32; ++i) {
// 	auto a = icon7::time::now();
		uint32_t executed = commandQueue.Execute(1024*1024);
// 	auto b = icon7::time::now();
// 	float ms = (b-a).msec();
// 	if (ms > 1) {
// 		printf("ms = %f -> %u\n", ms, executed);
// 	}
		
// 	}
	for (auto &h : hosts) {
		h->_InternalSingleLoopIteration();
	}
	FlushPendingPeers();
	RPCEnvironment::CheckForTimeoutFunctionCalls(this, 1);
}

void Loop::SetSleepBetweenUnlockedIterations(int32_t microseconds)
{
	microsecondsOfSleepBetweenIterations = microseconds;
}

void Loop::FlushPendingPeers()
{
	// TODO: check if pending peers are needed, when PeerData::Send calls _InternalFlushQueuedSends
	for (int32_t i=peerManager.peersToFlush.peers.size()-1; i>=0; --i) {
		uint32_t peerId = peerManager.peersToFlush.peers[i];
		PeerData *peer = peerManager.GetLocalPeerDataValid(peerId);
		if (peer) {
			if (peer->_InternalHasBufferedSends() || peer->_InternalHasQueuedSends()) {
				peer->_InternalOnWritable();
			}
			if ((!peer->_InternalHasBufferedSends()) && (!peer->_InternalHasQueuedSends())) {
				peerManager.peersToFlush.RemovePeerToFlush(peerId);
			}
		} else {
			peerManager.peersToFlush.RemovePeerToFlush(peerId);
		}
	}
}

void Loop::_InternalInsertPeerToFlush(PeerHandle peer)
{
	auto p = peer.GetLocalPeerData();
	if (p->_InternalHasQueuedSends() == false &&
		p->_InternalHasBufferedSends() == false) {
		peerManager.peersToFlush.InsertPeerToFlush(peer.id);
	}
}

PeerHandle Loop::_InternalGetRandomPeer()
{
	// TODO: optimize
	if (hosts.size() == 0) {
		return {};
	}
	int r = rand() % hosts.size(), i = 0;
	for (auto &h : hosts) {
		if (i == r) {
			return h->_InternalGetRandomPeer();
		} else {
			++i;
		}
	}
	return {};
}
} // namespace icon7
