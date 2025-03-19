/*
 *  This file is part of ICon7.
 *  Copyright (C) 2024 Marek Zalewski aka Drwalin
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

#include <bit>

#include "../include/icon7/MemoryPool.hpp"
#include "../include/icon7/ByteReader.hpp"
#include "../include/icon7/ByteWriter.hpp"

#include "../include/icon7/ByteBuffer.hpp"

namespace icon7
{
ByteBufferStorageHeader *ByteBufferStorageHeader::Allocate(uint32_t capacity)
{
	const uint32_t S = sizeof(ByteBufferStorageHeader);
	uint32_t trueCapacity = std::bit_ceil(capacity + S);
	if (trueCapacity < 64)
		trueCapacity = 64;

	void *vptr = MemoryPool::Allocate(trueCapacity).object;

	ByteBufferStorageHeader *ptr = (ByteBufferStorageHeader *)vptr;
	ptr->refCounter = 1;
	ptr->size = 0;
	ptr->offset = S;
	ptr->capacity = trueCapacity - S;
	return ptr;
}

void ByteBufferStorageHeader::Deallocate(ByteBufferStorageHeader *ptr)
{
	if (ptr->refCounter.load() == 0) {
		MemoryPool::Release(ptr, ptr->capacity + ptr->offset);
	}
}

ByteBufferStorageHeader *
ByteBufferStorageHeader::Reallocate(ByteBufferStorageHeader *ptr,
									uint32_t newCapacity)
{
	ByteBufferStorageHeader *ret =
		Allocate(newCapacity + ptr->offset - sizeof(ByteBufferStorageHeader));
	int32_t offsetDiff = ret->offset - ptr->offset;
	ret->offset -= offsetDiff;
	ret->capacity += offsetDiff;
	ret->size = ptr->size;
	ret->refCounter = 1;
	memcpy(ret->data(), ptr->data(), ptr->size);

	ptr->unref();
	return ret;
}

bitscpp::ByteReader<true> &
ByteBuffer::__ByteStream_op(bitscpp::ByteReader<true> &s)
{
	int32_t size = 0;
	s.op(size);
	if (size <= 0) {
		this->~ByteBuffer();
	} else {
		Init(size);
		s.op((uint8_t *)data(), size);
	}
	return s;
}

bitscpp::ByteWriter<icon7::ByteBuffer> &
ByteBuffer::__ByteStream_op(bitscpp::ByteWriter<icon7::ByteBuffer> &s)
{
	if (storage) {
		const uint32_t size = this->size();
		s.op(size);
		if (size > 0) {
			s.op((const uint8_t *)data(), size);
		}
	} else {
		const uint32_t size = 0;
		s.op(size);
	}
	return s;
}
} // namespace icon7
