// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cstdint>
#include <cassert>

#include <string>

#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/ByteWriter.hpp"
#include "../include/icon7/RpcName.hpp"

namespace icon7
{
RpcName::RpcName(const std::string &name) : str(name) { assert(name != ""); }
RpcName::RpcName(const char *name) : str(name) { assert(str != ""); }
RpcName::RpcName(uint32_t name) : uint(name)
{
	assert(name > 0);
	assert(name < 1024 * 1024);
}
RpcName::RpcName(int32_t name) : uint(name)
{
	assert(name > 0);
	assert(name < 1024 * 1024);
}

std::string RpcName::ToString() const
{
	if (str != "") {
		assert(uint == 0);
		return std::string("'") + str + "'";
	} else if (uint != 0) {
		assert(str == "");
		return std::string("[") + std::to_string(uint) + "]";
	} else {
		return "!!##INVALID_RPC_FUNCTION_NAME##!!";
	}
}

void RpcName::serialize(bitscpp::v2::ByteReader &reader)
{
	if (reader.is_next_integer()) {
		reader.op(uint);
		str = "";
	} else if (reader.is_next_string()) {
		reader.op(str);
		uint = 0;
	} else {
		reader.op(uint);
		uint = 0;
		str = "";
	}
}

void RpcName::serialize(bitscpp::v2::ByteWriter_ByteBuffer &writer) const
{
	if (str != "") {
		assert(uint == 0);
		writer.op(str);
	} else if (uint != 0) {
		assert(str == "");
		writer.op(uint);
	} else {
		writer.op_begin_object();
		writer.op_end_object();
	}
}
} // namespace icon7
