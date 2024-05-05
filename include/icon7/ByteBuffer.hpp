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

#ifndef ICON7_BYTE_BUFFER_HPP
#define ICON7_BYTE_BUFFER_HPP

#include <cstring>
#include <cstdint>

#include <atomic>

namespace icon7
{
struct ByteBufferStorageHeader {
	std::atomic<uint32_t> refCounter;
	uint32_t size;
	uint32_t offset;
	uint32_t capacity;

	inline void ref() { refCounter++; }
	inline void unref()
	{
		uint32_t n = refCounter.fetch_add(-1);
		if (n == 1) {
			Deallocate(this);
		}
	}

	inline uint8_t *data() { return ((uint8_t *)this) + offset; }

	static ByteBufferStorageHeader *Allocate(uint32_t capacity);
	static void Deallocate(ByteBufferStorageHeader *ptr);
	static ByteBufferStorageHeader *Reallocate(ByteBufferStorageHeader *ptr,
											   uint32_t newCapacity);
};

class ByteBuffer
{
public:
	ByteBuffer(const ByteBuffer &) = delete;
	ByteBuffer &operator=(const ByteBuffer &) = delete;

	inline ByteBuffer() { storage = nullptr; }
	inline ByteBuffer(ByteBuffer &&o)
	{
		storage = o.storage;
		o.storage = nullptr;
	}
	inline ByteBuffer(ByteBuffer &o)
	{
		storage = o.storage;
		if (storage)
			storage->ref();
	}
	~ByteBuffer();
	inline ByteBuffer(uint32_t initialCapacity)
	{
		storage = ByteBufferStorageHeader::Allocate(initialCapacity);
	}

	inline ByteBuffer &operator=(ByteBuffer &&o)
	{
		this->~ByteBuffer();
		storage = o.storage;
		o.storage = nullptr;
		return *this;
	}
	inline ByteBuffer &operator=(ByteBuffer &o)
	{
		this->~ByteBuffer();
		storage = o.storage;
		if (storage)
			storage->ref();
		return *this;
	}

	inline void Init(uint32_t initialCapacity)
	{
		this->~ByteBuffer();
		storage = ByteBufferStorageHeader::Allocate(initialCapacity);
	}
	
	inline void append(const uint8_t *src, uint32_t bytes)
	{
		reserve(size() + bytes);
		memcpy(data()+size(), src, bytes);
		storage->size += bytes;
	}

	inline uint8_t *data() { return storage->data(); }
	inline uint8_t *const data() const { return storage->data(); }
	inline size_t size() const { return storage->size; }
	inline void resize(size_t newSize)
	{
		if (capacity() < newSize) {
			reserve(newSize);
		}
		storage->size = newSize;
	}
	inline size_t capacity() const { return storage->capacity; }
	void reserve(size_t newCapacity)
	{
		if (newCapacity > storage->capacity) {
			storage = ByteBufferStorageHeader::Reallocate(storage, newCapacity);
		}
	}

private:
	ByteBufferStorageHeader *storage = nullptr;
};
} // namespace icon7

#endif
