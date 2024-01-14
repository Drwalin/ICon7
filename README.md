
# ICon7

Provides async interface for Remote Procedure Call (RPC), agnostic towards
networking library. The
library itself does not provide encryption or reliability mechanisms but require
the underlying library to provide those themselfs if needed.

## Remote method invocation

Remote method invocation is not implemented any more. It can be simply
reimplemented as wrapper around RPC.

## Limits

Unreliable message size is limited to MTU, which usually is around 1500 bytes,
but for safety reasons it shouldn't be more than around 768 or 1024 bytes.

Reliable messages (with headers) should be smaller than 64KiB (65536 bytes), but
size is limited by 2^60 (1 EiB). Size of messages may be limited by underlying
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

Header has variadic size of from 1 byte up to 8 bytes. The following figure
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
    - 00 - header 1 byte, body size of from 1 to 16 bytes
    - 01 - header 2 bytes, body size of from 1 to 4096 bytes
    - 10 - header 4 bytes, body size of from 1 byte to 256 MiB
    - 11 - header 8 bytes, body size of from 1 byte to 1 EiB 
yy - determines type of RPC message:
    - 00 - procedure call, without any form of feedback
    - 01 - procedure call, callee awaits for feedback
    - 10 - function call, without feedback
    - 11 - function call, callee awaits returned value

zzzz...zz - size of body of message, stored in little endian. Effectively to
    extract size of message body one needs to get little endian integer from
    whole header and then bit shift it by 4, then add 1 to result:
``` C
    uint8_t header[8] = ...;
    uint8_t sizeOfSize = header[0] & 0b11;
    uint64_t bodySize = 0;
    if (sizeOfSize == 10) {
        uint32_t tmp = get_little_endian_uint32(header);
        bodySize = (tmp>>4) + 1;
    } else if ...
```

### structure of message body

If message is a function/procedure then first four bytes are taken by call id
of size of 4 bytes, stored in little endian order.

Then there is a function name string NULL terminated.

Finally there are function arguments, endcoded in little endian (integers),
and floats are stored in little endian with IEEE 754 standard single or double
precision.

Arrays, sets and maps are stored with number of elements as first element
uint32\_t. Maps elements are stored as pairs of (key, value).

String are stored as NULL-terminated utf-8 strings.

