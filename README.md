
# ICon7

Provides async interface for Remote Procedure Call (RPC), agnostic towards
networking library. The
library itself does not provide encryption or reliability mechanisms but require
the underlying library to provide those themselfs if needed.

## Remote method invocation

Remote method invocation is not implemented any more. It can be simply
reimplemented as wrapper around RPC.

## Limits

Unreliable messages are not implemented yet!

Unreliable message size is limited to MTU, which usually is around 1500 bytes,
but for safety reasons it shouldn't be more than around 768 or 1024 bytes.

Reliable messages (with headers) should be smaller than 64 KiB, but
size is limited by 2^28 (256 MiB). Size of messages may be limited by underlying
networking library more.

## Vulnerabilities

The same as underlying networking library.

## Requirements

- cmake
- OpenSSL
- C++ 17 compliant compiler

## How to compile

```bash
mkdir build
cmake ..
make
```

## Usage

### Examples

Examples are provided in `examples/` directory. Examples called `example1.cpp`
and `example6.cpp` work only on linux. It is recommended to learn on
chat\_server/chat\_client or echo\_server/echo\_client.

## Protocol description

Protocol is fully asynchronous and duplex, but requires the underlying network
library to be received as continous data in case of stream protocols.

For sake of simplicity call, return calls, messages, control messages (if
implemented in future) will be named as messages.

Each message have header that describes it's size and some aditional information.
Within one packet (definition depends on underlying library) can be as many
message as can fit inside.

In case of streaming protocols, such as TCP, messages are distinguished by their
headers.

### Structure of message header

Header has variadic size of from 1 byte up to 4 bytes. The following figure
represents header structure:

```
         Header
  ___________________
 /                   \
+----------+----------+------------------+
| zzzzyyxx | zz....zz | ... message body |
+----------+----------+------------------+
```

xx - (2 least significant bits of first byte) - determine size of header:
```
    - 00 - header 1 byte, body size from 1 to 16 bytes
    - 01 - header 2 bytes, body size from 1 to 4 KiB
    - 10 - header 3 bytes, body size from 1 byte to 1 MiB
    - 11 - header 4 bytes, body size from 1 byte to 256 MiB 
```

yy - determines type of RPC message:
- 00 - function/procedure call without feedback
- 01 - function/procedure call where callee awaits returned value
           (or signal of execution finished in case of `void` return type)
- 10 - return feedback (from callee to caller)
- 11 - Controll sequence. See Controll packet structure.
    Then first body byte values between 0x00-0x7F are
           reserved for future use; it's values between 0x80-0xFF are to be used
           by underlying networking library.

zzzz...zz - size of body of message, stored in little endian. Effectively to
    extract size of message body one needs to get little endian integer from
    whole header and then bit shift it by 4, then add 1 to result:
``` C
    uint8_t header[4] = ...;
    uint8_t sizeOfSize = header[0] & 0b11;
    uint32_t bodySize = 0;
    if (sizeOfSize == 0b11) {
        uint32_t tmp = get_little_endian_uint32(header);
        bodySize = (tmp>>4) + 1;
    } else if ...
```

### Controll packet structure
```
         Header           1 byte
  ___________________   ___________
 /                   \ /           \
+----------+----------+-------------+------------------+
| zzzz11xx | zz....zz | vector call | ... message body |
+----------+----------+-------------+------------------+
```

Vector call byte values:
- 0x00-0x7F - reserved for future use in protocol
- 0x80-0xFF - reserved for use for underlying backend implementation

### structure of message body

```
+-----------+------------------------------+------------------+
| Header... | optional 4 bytes feedback id | ... message body |
+-----------+------------------------------+------------------+
```

If message is a function/procedure with return feedback then first 4 bytes are
taken by call id of size of 4 bytes, stored in little endian order.

Next there is a function name string NULL terminated.

Finally there are function arguments, endcoded in little endian (integers),
and floats are stored in little endian with IEEE 754 standard single or double
precision.

Arrays, sets and maps are stored with number of elements as first element
uint32\_t. Maps elements are stored as pairs of (key, value).

String are stored as NULL-terminated utf-8 strings.

### Return feedback

A message that is a return feedback of function call, as data has first 4 bytes
call id.

Then there may be optional returned values.

## Implemented backends

### uSockets tcp

### uSocket tcp+udp (not implemented yet)

Custom protocol for controll sequence:

Vector call byte in controll sequence values:

Vector call `0x80` is sent through tcp connection to the other peer during
handshake. Packet data:
| offset | bytes | description                            |
|--------|-------|----------------------------------------|
| 0      | 1     | Vector call of value `0x80`            |
| 1      | 4     | This peer receiving identity           |
|        |       | The following for SSL connections only |
| 5      | 32    | This peer encryption key               |

Vector call `0x81` is sent as acknowledgement after receiving first udp packet
from client by server. Can be sent by either tcp or udp (preferably by UDP to
fully establish dublex routing in NAT routers). Packet data:
| offset | bytes | description                 |
|--------|-------|-----------------------------|
| 0      | 1     | Vector call of value `0x81` |

#### UDP protocol

Packet data:
| offset | bytes  | description                                      |
|--------|--------|--------------------------------------------------|
| 0      | 4      | Endpoint receiving identity                      |
| 4      | 4      | Packet id in sequence                            |
|        |        | The following for SSL connections only           |
| 8      | XXX    | Encrypted packet payload encrypted with chacha20 |
|        |        | using Packet id in sequence as last 4 bytes of   |
|        |        | nonce and endpoint receiving identity as AAD     |
| XXX+8  | 16 (?) | Packet MAC poly1305                              |
|        |        | For not encrypted connections                    |
| 8      | XXX    | Unencrypted packet payload                       |



