// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_RPC_NAME_HPP
#define ICON7_RPC_NAME_HPP

#include <cstdint>
#include <cassert>

#include <string>

#include "Forward.hpp"

namespace icon7
{
struct RpcName {
	RpcName() = default;
	RpcName(const std::string &name);
	RpcName(const char *name);
	RpcName(uint32_t name);
	RpcName(int32_t name);

	inline bool IsString() const
	{
		assert(*this);
		return str != "";
	}
	inline bool IsUint() const
	{
		assert(*this);
		return uint != 0;
	}

	inline operator bool() const
	{
		return (str != "" ? 1 : 0) ^ (uint != 0 ? 1 : 0);
	}

	std::string ToString() const;

	void serialize(bitscpp::v2::ByteReader &reader);
	void serialize(bitscpp::v2::ByteWriter_ByteBuffer &writer) const;

	std::string str = "";
	uint32_t uint = 0;
};
} // namespace icon7

#endif
