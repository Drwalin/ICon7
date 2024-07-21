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

#ifndef ICON7_FORWARD_DECLARATIONS_HPP
#define ICON7_FORWARD_DECLARATIONS_HPP

namespace moodycamel
{
template <typename T, typename Traits> class ConcurrentQueue;
}

namespace bitscpp
{
template <typename T> class ByteWriter;
template <bool V> class ByteReader;
} // namespace bitscpp

namespace icon7
{
class ByteWriter;
class ByteReader;
class ByteBuffer;
class CommandExecutionQueue;
class Peer;
class Host;
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
namespace tcp
{
class Peer;
class Host;
} // namespace tcp
} // namespace uS
} // namespace icon7

#endif
