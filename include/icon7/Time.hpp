// Copyright (C) 2023-2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_TIME_HPP
#define ICON7_TIME_HPP

#include "../../drwalin_util/include/drwalin_util/time.hpp"

#include "Forward.hpp"

namespace icon7
{
namespace time
{
using namespace dutl::time;
using Point = point;
using Diff = diff;
using Timestamp = timestamp;
}
} // namespace icon7

namespace bitscpp
{
namespace v2
{
void serialize(ByteReader &s, icon7::time::Point &v);
void serialize(ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Point &v);

void serialize(ByteReader &s, icon7::time::Diff &v);
void serialize(ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Diff &v);

void serialize(ByteReader &s, icon7::time::Timestamp &v);
void serialize(ByteWriter<icon7::ByteBufferWritable> &s, const icon7::time::Timestamp &v);
} // namespace v2
} // namespace bitscpp
#endif
