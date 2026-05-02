// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_PEERS_TO_FLUSH_HPP
#define ICON7_PEERS_TO_FLUSH_HPP

#include <vector>

#include "Forward.hpp"

namespace icon7
{
class PeersToFlush {
public:
	void InsertPeerToFlush(uint32_t peer);
	void RemovePeerToFlush(uint32_t peer);

protected:
	friend class Loop;
	friend class Host;
	std::vector<uint32_t> peers;
	std::vector<int32_t> offsets;
};
}

#endif
