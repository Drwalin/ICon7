// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Forward.hpp"
#include "../include/icon7/ByteBuffer.hpp"
#include "../include/icon7/ByteWriter.hpp"
#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/Time.hpp"

namespace bitscpp
{
namespace v2
{
void serialize(ByteReader &s, icon7::time::Point &v) { s.op(v.ns); }
void serialize(v2::ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Point &v) { s.op(v.ns); }

void serialize(ByteReader &s, icon7::time::Diff &v) { s.op(v.ns); }
void serialize(v2::ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Diff &v) { s.op(v.ns); }

void serialize(ByteReader &s, icon7::time::Timestamp &v) { s.op(v.ns); }
void serialize(v2::ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Timestamp &v)
{
	s.op(v.ns);
}
} // namespace v2
} // namespace bitscpp
