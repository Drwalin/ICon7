// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
// 
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_COMMAND_HPP
#define ICON7_COMMAND_HPP

#include <memory>
#include <coroutine>
#include <type_traits>

#include "Debug.hpp"
#include "Flags.hpp"
#include "MemoryPool.hpp"
#include "ByteReader.hpp"

namespace icon7
{
class Host;
class Peer;
class MessageConverter;

class CommandExecutionQueue;

/*
 * True pointer is obtained after setting low 4 bits of address to 0.
 * This low-bit of pointer tagging is not implemented.
 */
enum CommandPtrLowBitValues : uint8_t {
	/*
	 * Pointer to class Command
	 */
	REGULAR_COMMAND = 0,

	/*
	 * Pointer is value of std::coroutine_handle<>
	 */
	COROUTINE_HANDLE = 1,

	// 	/*
	// 	 * Treating pointer as pointer to function: void(void)
	// 	 */
	// 	FUNCTION_POINTER = 2,
};

class Command
{
public:
	inline Command() {}

	Command(Command &&other) = delete;
	Command(Command &other) = delete;
	Command(const Command &other) = delete;

	Command &operator=(Command &&other) = delete;
	Command &operator=(Command &other) = delete;
	Command &operator=(const Command &other) = delete;

	virtual void Execute() = 0;

	uint32_t _commandSize = 0;

	virtual ~Command();
};

template <typename T = Command> class CommandHandle
{
public:
	CommandHandle() : _com(nullptr) {}
	template <typename T2> CommandHandle(CommandHandle<T2> &&o)
	{
		static_assert(std::is_base_of_v<T, T2>);
		_com = o._com;
		o._com = nullptr;
	}
	CommandHandle(CommandHandle<std::coroutine_handle<>> &&o)
	{
		static_assert(std::is_same_v<T, Command> ||
					  std::is_same_v<T, std::coroutine_handle<>>);
		_com = o._com;
		o._com = nullptr;
	}
	CommandHandle(CommandHandle &o) = delete;
	CommandHandle(const CommandHandle &o) = delete;

	inline ~CommandHandle() { Destroy(); }

	CommandHandle &operator=(CommandHandle &o) = delete;
	CommandHandle &operator=(const CommandHandle &o) = delete;
	template <typename T2> CommandHandle &operator=(CommandHandle<T2> &&o)
	{
		if (_com != nullptr) {
			LOG_FATAL(
				"Cannot move CommandHandle<> into not empty CommandHandle<>");
			return *this;
		}
		static_assert(std::is_base_of_v<T, T2>);
		this->~CommandHandle();
		new (this) CommandHandle<T>(std::move(o));
		return *this;
	}

	void Destroy()
	{
		const uint8_t type = _int & 0x7;
		_int ^= (uintptr_t)type;
		if (_com != nullptr) {
			switch (type) {
			case REGULAR_COMMAND:
				MemoryPool::ReleaseTyped(_com, _com->_commandSize);
				break;
			case COROUTINE_HANDLE:
				std::coroutine_handle<>::from_address(_coro).destroy();
				break;
			}
			_com = nullptr;
		}
	}

	template <typename... Args>
	inline static CommandHandle<T> Create(Args &&...args)
	{
		CommandHandle<T> com;
		com._com = MemoryPool::AllocateTyped<T>(std::move(args)...);
		if ((((uintptr_t)com._com) & 0x7) != 0) {
			LOG_FATAL("Allocator does not return 16-byte align memory");
		}
		com._com->_commandSize = sizeof(T);
		return com;
	}

	inline static CommandHandle<T> Create(std::coroutine_handle<> handle)
	{
		CommandHandle<T> com;
		com._coro = handle.address();
		if ((((uintptr_t)com._com) & 0x7) != 0) {
			LOG_FATAL("Allocator does not return 16-byte align memory");
		}
		com._int |= COROUTINE_HANDLE;
		return com;
	}

	inline void Execute()
	{
		const uint8_t type = _int & 0x7;
		_int ^= (uintptr_t)type;
		if (_com != nullptr) {
			switch (type) {
			case REGULAR_COMMAND:
				_com->Execute();
				MemoryPool::ReleaseTyped(_com, _com->_commandSize);
				break;
			case COROUTINE_HANDLE:
				std::coroutine_handle<>::from_address(_coro)();
				break;
			}
			_com = nullptr;
		}
	}

	inline bool IsValid() const { return _com != nullptr; }

	inline T &operator*(int) { return *(T *)_com; }

	inline T *operator->() { return (T *)_com; }

	friend class CommandExecutionQueue;

public:
	union {
		Command *_com;
		void *_coro;
		uintptr_t _int;
	};
};

template <>
inline std::coroutine_handle<> &
CommandHandle<std::coroutine_handle<>>::operator*(int) = delete;

template <>
inline std::coroutine_handle<> *
CommandHandle<std::coroutine_handle<>>::operator->() = delete;

namespace commands
{
class ExecuteOnPeer : public Command
{
public:
	virtual ~ExecuteOnPeer() {}
	ExecuteOnPeer() {}
	ExecuteOnPeer(std::shared_ptr<Peer> &peer) : peer(peer) {}

	std::shared_ptr<Peer> peer;
};

class ExecuteOnHost : public Command
{
public:
	virtual ~ExecuteOnHost() {}
	ExecuteOnHost() {}
	ExecuteOnHost(Host *host) : host(host) {}

	Host *host = nullptr;
};

class ExecuteBooleanOnHost : public ExecuteOnHost
{
public:
	virtual ~ExecuteBooleanOnHost() {}
	ExecuteBooleanOnHost() {}
	ExecuteBooleanOnHost(Host *host, bool result)
		: ExecuteOnHost(host), result(result)
	{
	}

	bool result = false;
};

namespace internal
{
class ExecuteRPC : public Command
{
public:
	virtual ~ExecuteRPC() {}
	ExecuteRPC(ByteReader &&reader) : reader(std::move(reader)) {}

	std::shared_ptr<Peer> peer;
	ByteReader reader;
	MessageConverter *messageConverter = nullptr;
	Flags flags = 0;
	uint32_t returnId = 0;

	virtual void Execute() override;
};

class ExecuteAddPeerToFlush : public Command
{
public:
	virtual ~ExecuteAddPeerToFlush() {}
	ExecuteAddPeerToFlush() {}
	ExecuteAddPeerToFlush(std::shared_ptr<Peer> peer, Host *host)
		: peer(peer), host(host)
	{
	}

	std::shared_ptr<Peer> peer;
	Host *host = nullptr;

	virtual void Execute() override;
};

class ExecuteListen : public Command
{
public:
	virtual ~ExecuteListen() {}

	ExecuteListen() {}

	std::string address;
	IPProto ipProto;
	Host *host = nullptr;
	uint16_t port = 0;

	CommandHandle<ExecuteBooleanOnHost> onListen;
	CommandExecutionQueue *queue = nullptr;

	virtual void Execute() override;
};

class ExecuteConnect : public Command
{
public:
	virtual ~ExecuteConnect() {}
	ExecuteConnect() {}

	Host *host = nullptr;
	std::string address;
	uint16_t port = 0;

	CommandExecutionQueue *executionQueue;
	CommandHandle<ExecuteOnPeer> onConnected;

	virtual void Execute() override;
};

class ExecuteDisconnect : public Command
{
public:
	virtual ~ExecuteDisconnect() {}
	ExecuteDisconnect() {}

	std::shared_ptr<Peer> peer;

	virtual void Execute() override;
};
} // namespace internal
} // namespace commands
} // namespace icon7

#endif
