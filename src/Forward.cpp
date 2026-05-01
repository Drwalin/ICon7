// Copyright (C) 2026 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Host.hpp"
#include "../include/icon7/Loop.hpp"

#include "../include/icon7/Forward.hpp"

namespace icon7
{
PeerData *PeerHandle::GetLocalPeerData()
{
	// Should be invoked very rarely
	return loop->GetLocalPeerData(*this);
}
std::shared_ptr<Peer> PeerHandle::GetSharedPeer()
{
	// Should be invoked very rarely
	return loop->GetSharedPeer(*this);
}
Loop *PeerHandle::GetLoop()
{
	// Should be invoked very rarely
	return loop;
}
Host *PeerHandle::GetHost()
{
	// Should be invoked very rarely
	return loop->GetSharedHost(*this);
}
} // namespace icon7
