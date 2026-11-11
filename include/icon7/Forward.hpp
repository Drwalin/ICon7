// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_FORWARD_DECLARATIONS_HPP
#define ICON7_FORWARD_DECLARATIONS_HPP

#include "../../bitscpp/include/bitscpp/BitscppForward.hpp" // IWYU pragma: export

namespace moodycamel
{
template <typename T, typename Traits> class ConcurrentQueue;
struct ConsumerToken;
struct ProducerToken;
} // namespace moodycamel

namespace icon7
{
struct ByteBufferStorageHeader;
class ByteBufferReadable;
class ByteBufferWritable;
class CommandExecutionQueue;
struct CoroutineSchedulable;
class MultiBatchWriter;
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
struct RpcName;

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
class ByteWriter;
class ByteReader;
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

#include "PeerHandle.hpp" // IWYU pragma: export

#endif
