/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
 *
 *  ICon7 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdarg>

#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <regex>

#include "../include/icon7/Command.hpp"
#include "../include/icon7/Peer.hpp"

#include "../include/icon7/Host.hpp"

namespace icon7
{
void Debug(const char *file, int line, const char *function, const char *fmt,
		   ...)
{
	static std::atomic<int> globID = 1;
	thread_local static int id = globID++;

	std::string funcName = function;
	funcName = std::regex_replace(funcName, std::regex("\\[[^\\[\\]]+\\]"), "");
	// 	funcName = std::regex_replace(funcName, std::regex("\\[[^\\[\\]]+\\]"),
	// ""); 	funcName = std::regex_replace(funcName,
	// std::regex("\\[[^\\[\\]]+\\]"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	funcName = std::regex_replace(funcName,
								  std::regex(" ?(static|virtual|const) ?"), "");
	// 	funcName = std::regex_replace(funcName, std::regex("[^ ]* "), "");
	funcName =
		std::regex_replace(funcName, std::regex("\\([^\\(\\)]+\\)"), "()");
	funcName = std::regex_replace(
		funcName, std::regex(".*[: >]([a-zA-Z0-9_]*::[a-z0-9A-Z_]*)+\\(.*"),
		"$1");
	funcName += "()";

	va_list va;
	va_start(va, fmt);
	static std::mutex mutex;
	std::lock_guard lock(mutex);
	fprintf(stdout, "%s:%i\t\t%s\t\t [ %2i ] \t ", file, line, funcName.c_str(),
			id);
	vfprintf(stdout, fmt, va);
	fprintf(stdout, "\n");
	fflush(stdout);
}

Host::Host()
{
	userData = 0;
	userPointer = nullptr;
	onConnect = nullptr;
	onDisconnect = nullptr;
	asyncRunnerFlags = 0;
}

Host::~Host() { _InternalDestroy(); }

void Host::WaitStopRunning()
{
	QueueStopRunning();
	while (IsRunningAsync()) {
		QueueStopRunning();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void Host::_InternalDestroy()
{
	DisconnectAllAsync();
	DispatchAllEventsFromQueue(0x7FFFFFFF);
	while (commandQueue.HasAny()) {
		DispatchAllEventsFromQueue(0x7FFFFFFF);
	}
	WaitStopRunning();
	DispatchAllEventsFromQueue(0x7FFFFFFF);
	while (commandQueue.HasAny()) {
		DispatchAllEventsFromQueue(0x7FFFFFFF);
	}
}

void Host::DisconnectAllAsync()
{
	this->EnqueueCommand(commands::ExecuteOnHost{
		this, nullptr, [](Host *host, void *) { host->DisconnectAll(); }});
}

void Host::DisconnectAll()
{
	DEBUG("");
	for (auto &p : peers) {
		p->Disconnect();
	}
}

uint32_t Host::DispatchAllEventsFromQueue(uint32_t maxEvents)
{
	const uint32_t MAX_DEQUEUE_COMMANDS = 128;
	maxEvents = std::min(maxEvents, MAX_DEQUEUE_COMMANDS);
	Command commands[MAX_DEQUEUE_COMMANDS];
	const uint32_t dequeued =
		commandQueue.TryDequeueBulkAny(commands, maxEvents);

	for (uint32_t i = 0; i < dequeued; ++i) {
		commands[i].Execute();
		commands[i].~Command();
	}
	return dequeued;
}

std::future<Peer *> Host::ConnectPromise(std::string address, uint16_t port)
{
	std::promise<Peer *> *promise = new std::promise<Peer *>();
	commands::ExecuteOnPeer onConnected;
	onConnected.userPointer = promise;
	onConnected.function = [](auto peer, auto data, auto userPointer) {
		DEBUG("");
		std::promise<Peer *> *promise = (std::promise<Peer *> *)(userPointer);
		promise->set_value(peer);
		delete promise;
	};
	auto future = promise->get_future();
	Connect(address, port, std::move(onConnected), nullptr);
	return future;
}

std::future<bool> Host::ListenOnPort(uint16_t port)
{
	std::promise<bool> *promise = new std::promise<bool>();
	commands::ExecuteBooleanOnHost callback;
	callback.host = this;
	callback.userPointer = promise;
	callback.function = [](Host *host, bool result, void *userPointer) {
		DEBUG("");
		std::promise<bool> *promise = (std::promise<bool> *)(userPointer);
		promise->set_value(result);
		delete promise;
	};
	auto future = promise->get_future();
	ListenOnPort(port, std::move(callback), nullptr);
	return future;
}

void Host::Connect(std::string address, uint16_t port)
{
	commands::ExecuteOnPeer com;
	com.function = [](auto a, auto b, auto c) {};
	Connect(address, port, std::move(com), nullptr);
}

void Host::EnqueueCommand(Command &&command)
{
	commandQueue.EnqueueCommand(std::move(command));
}

enum RunnerFlags : uint32_t { RUNNING = 1, QUEUE_STOP = 2 };

void Host::RunAsync()
{
	asyncRunnerFlags = 0;
	asyncRunner = std::thread([this]() {
		asyncRunnerFlags |= RUNNING;
		while (IsQueuedStopAsync() == false) {
			SingleLoopIteration();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		asyncRunnerFlags &= ~RUNNING;
	});
	asyncRunner.detach();
}

void Host::QueueStopRunning()
{
	if (IsRunningAsync()) {
		asyncRunnerFlags |= QUEUE_STOP;
	}
}

bool Host::IsQueuedStopAsync() { return asyncRunnerFlags & QUEUE_STOP; }

bool Host::IsRunningAsync() { return asyncRunnerFlags & RUNNING; }

void Host::SingleLoopIteration()
{
	DispatchAllEventsFromQueue(16);
	int i = 0;
	std::vector<std::shared_ptr<Peer>> toRemoveFromQueue;
	toRemoveFromQueue.reserve(32);
	for (auto p : peersToFlush) {
		if (p->IsReadyToUse()) {
			p->_InternalOnWritable();
			toRemoveFromQueue.emplace_back(p);
		}
		++i;
		if (i == 16)
			break;
	}
	for (auto &p : toRemoveFromQueue) {
		if (p->_InternalHasQueuedSends() == false) {
			peersToFlush.erase(p);
		}
	}
}

void Host::InsertPeerToFlush(Peer *peer)
{
	EnqueueCommand(
		commands::ExecuteAddPeerToFlush{peer->shared_from_this(), this});
}

void Host::_InternalInsertPeerToFlush(Peer *peer)
{
	peersToFlush.insert(peer->shared_from_this());
}

void Host::ForEachPeer(void(func)(icon7::Peer *))
{
	for (auto &p : peers) {
		func(p.get());
	}
}

void Host::ForEachPeer(std::function<void(icon7::Peer *)> func)
{
	for (auto &p : peers) {
		func(p.get());
	}
}

void Initialize() {}
void Deinitialize() {}

} // namespace icon7
