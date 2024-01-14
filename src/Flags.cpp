/*
 *  This file is part of ICon7.
 *  Copyright (C) 2023 Marek Zalewski aka Drwalin
 *
 *  ICon7 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ICon7 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstring>

#include <steam/steamnetworkingtypes.h>

#include <bitscpp/Endianness.hpp>

#include "../include/icon7/Flags.hpp"

namespace icon7
{
void Flags::GetNetworkOrder(void *ptr) const
{
	uint32_t v = bitscpp::HostToNetworkUint(field);
	memcpy(ptr, &v, sizeof(uint32_t));
}

uint32_t Flags::GetSteamFlags() const
{
	uint32_t ret = 0;
	if (field & FLAG_RELIABLE)
		ret |= k_nSteamNetworkingSend_Reliable;
	if (field & FLAG_NO_NAGLE)
		ret |= k_nSteamNetworkingSend_NoNagle;
	return ret;
}
} // namespace icon7
