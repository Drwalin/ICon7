// Copyright (C) 2023-2024 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_DNS_HPP
#define ICON7_DNS_HPP

#include <cinttypes>

#include <string>

#include "Flags.hpp"

// NOLINTBEGIN
namespace icon7
{
class AddressInfo
{
public:
	AddressInfo();
	~AddressInfo();

	inline const static size_t ADDRESS4_STORAGE_SIZE = 8;
	inline const static size_t ADDRESS6_STORAGE_SIZE = 28;
	inline const static size_t ADDRESS_STORAGE_SIZE = ADDRESS6_STORAGE_SIZE;

	bool Populate(const std::string address, const uint16_t port,
				  const IPProto proto);
	void Clear();

	bool CopyAddressTo(void *ptr);

private:
	struct sockaddr_storage *Address();

	uint8_t addressStorage[ADDRESS_STORAGE_SIZE];
	IPProto proto;
};

class DomainNameSystemResolution
{
public:
};
} // namespace icon7
// NOLINTEND

#endif
