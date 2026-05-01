// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FORWARD_DECLARATIONS_HPP
#define ICON7_FORWARD_DECLARATIONS_HPP

#include <memory>

namespace moodycamel
{
template <typename T, typename Traits> class ConcurrentQueue;
struct ConsumerToken;
struct ProducerToken;
} // namespace moodycamel

namespace bitscpp
{
namespace v2
{
class ByteWriter_ByteBuffer;
class ByteReader;
} // namespace v2
} // namespace bitscpp

namespace icon7
{
class ByteWriter;
class ByteReader;
struct ByteBufferStorageHeader;
class ByteBufferReadable;
class ByteBufferWritable;
class CommandExecutionQueue;
struct CoroutineSchedulable;
class Loop;
class Host;
class Peer;
class PeerData;
class Command;
template <typename T> class CommandHandle;
class CommandsBufferHandler;
class CommandsBuffer;
struct ConcurrentQueueDefaultTraits;
class FrameDecoder;
class MessageConverter;
class RPCEnvironment;
struct SendFrameStruct;

namespace commands
{
class ExecuteOnPeer;
class ExecuteOnHost;
class ExecuteBooleanOnHost;

namespace internal
{
class ExecuteRPC;
class ExecuteAddPeerToFlush;
class ExecuteListen;
class ExecuteConnect;
class ExecuteDisconnect;
} // namespace internal
} // namespace commands

class ExecuteReturnCallbackBase;
} // namespace icon7

namespace icon7
{
namespace uS
{
class Loop;
namespace tcp
{
class Host;
class Peer;
} // namespace tcp
} // namespace uS
} // namespace icon7

namespace icon7
{
struct PeerHandle {
	std::shared_ptr<PeerData> ptr;
	Loop *loop = nullptr;
	inline bool operator==(const PeerHandle &r) const {
		return ptr == r.ptr && loop == r.loop;
	}
	inline bool operator!=(const PeerHandle &r) const {
		return ptr != r.ptr || loop != r.loop;
	}
	inline operator bool() const {
		return ptr != nullptr && loop != nullptr;
	}
	
	// Should be invoked very rarely
	PeerData *GetLocalPeerData();
	std::shared_ptr<Peer> GetSharedPeer();
	Loop *GetLoop();
	Host *GetHost();
};
}

#endif
