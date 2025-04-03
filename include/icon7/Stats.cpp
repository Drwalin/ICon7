// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#include "../include/icon7/Time.hpp"

#include "../include/icon7/Stats.hpp"

namespace icon7
{
PeerStats::PeerStats() { startTimestamp = icon7::time::GetTimestamp(); }
} // namespace icon7
