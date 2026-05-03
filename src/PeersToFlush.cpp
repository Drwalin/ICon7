// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include <cassert>

#include "../include/icon7/PeersToFlush.hpp"

namespace icon7
{
void PeersToFlush::InsertPeerToFlush(uint32_t peer)
{
// 	return;
	if (offsets.size() <= peer) {
		offsets.resize(peer + 1, -1);
	}
	if (offsets[peer] >= 0) {
		return;
	}
	offsets[peer] = peers.size();
	peers.push_back(peer);
}

void PeersToFlush::RemovePeerToFlush(uint32_t peer)
{
	if (offsets.size() <= peer) {
		return;
	}
	if (offsets[peer] < 0) {
		return;
	}
	assert(peers.size() > 0);
	int32_t off = offsets[peer];
	offsets[peer] = -1;
	if (off+1 != peers.size()) {
		peers[off] = peers.back();
		offsets[peers[off]] = off;
	}
	peers.pop_back();
}
}
