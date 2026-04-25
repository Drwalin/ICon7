// Copyright (C) 2023-2025 Marek Zalewski aka Drwalin
//
// This file is part of ICon7 project under MIT License
// You should have received a copy of the MIT License along with this program.

#ifndef ICON7_BYTE_BUFFER_HPP
#define ICON7_BYTE_BUFFER_HPP

#include <cstring>
#include <cstdint>
#include <cassert>

#include <atomic>

#include "Flags.hpp"
#include "MemoryPool.hpp"

namespace bitscpp
{
namespace v2
{
class ByteReader;
class ByteWriter_ByteBuffer;
} // namespace v2
} // namespace bitscpp

namespace icon7
{
struct ByteBufferStorageHeader {
	std::atomic<uint32_t> refCounter;
	uint32_t size;
	uint32_t offset;
	uint32_t capacity;
	Flags flags;

	inline const static uint32_t INITIAL_DATA_OFFSET = 32;

	inline void ref() { ++refCounter; }
	inline void unref()
	{
		uint32_t n = --refCounter;
		if (n == 0) {
			MemoryPool::Release(this, capacity + offset);
		}
	}

	inline uint8_t *data() { return ((uint8_t *)this) + offset; }
};

class ByteBufferWritable
{
public:
	inline ByteBufferWritable()
	{
		buffer = nullptr;
		_size = 0;
		_capacity = 0;
		_offset = 0;
		_flags = 0;
	}
	inline ByteBufferWritable(ByteBufferWritable &&o)
	{
		buffer = o.buffer;
		_size = o._size;
		_capacity = o._capacity;
		_offset = o._offset;
		_flags = o._flags;
		o.buffer = nullptr;
		o._size = 0;
		o._capacity = 0;
		o._offset = 0;
		o._flags = 0;
	}
	inline ByteBufferWritable(const ByteBufferWritable &o)
	{
		if (o.buffer == nullptr) {
			buffer = nullptr;
			_size = 0;
			_capacity = 0;
			_offset = 0;
			_flags = 0;
			return;
		}
		_size = o._size;
		_offset = o._offset;
		_flags = o._flags;
		auto buf = MemoryPool::Allocate(o._capacity + o._offset);
		buffer = ((uint8_t *)buf.object) + o._offset;
		_capacity = buf.capacity - _offset;
	}
	inline ByteBufferWritable(ByteBufferWritable &o)
		: ByteBufferWritable((const ByteBufferWritable &)o)
	{
	}
	inline ByteBufferWritable(uint32_t initialCapacity) : ByteBufferWritable()
	{
		buffer = nullptr;
		_capacity = initialCapacity;
		_offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
		_size = 0;
		_flags = 0;
		auto buf = MemoryPool::Allocate(_capacity + _offset);
		buffer = ((uint8_t *)buf.object) + _offset;
		_capacity = buf.capacity - _offset;
	}

	~ByteBufferWritable()
	{
		if (buffer) {
			assert(_capacity);
			assert(_offset);
			MemoryPool::Release(buffer - _offset, _capacity + _offset);
			buffer = nullptr;
			_size = 0;
			_capacity = 0;
			_offset = 0;
			_flags = 0;
		}
	}

	inline ByteBufferWritable &operator=(ByteBufferWritable &&o)
	{
		this->~ByteBufferWritable();
		new (this) ByteBufferWritable(std::move(o));
		return *this;
	}
	inline ByteBufferWritable &operator=(const ByteBufferWritable &o)
	{
		this->~ByteBufferWritable();
		new (this) ByteBufferWritable(o);
		return *this;
	}
	inline ByteBufferWritable &operator=(ByteBufferWritable &o)
	{
		this->~ByteBufferWritable();
		new (this) ByteBufferWritable(o);
		return *this;
	}

	inline void clear()
	{
		_size = 0;
		if (buffer) {
			buffer -= _offset;
			_offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
			buffer += _offset;
		} else {
			_capacity = 0;
			_offset = 0;
		}
		_flags = 0;
	}

	inline bool valid() const { return buffer != nullptr; }

	inline void append(const uint8_t *src, uint32_t bytes)
	{
		reserve(_size + bytes);
		memcpy(buffer + _size, src, bytes);
		_size += bytes;
	}

	inline void push_back(uint8_t byte)
	{
		reserve(_size + 1);
		buffer[_size] = byte;
		++_size;
	}

	inline void write(const uint8_t *data, uint32_t bytes)
	{
		append(data, bytes);
	}

	inline uint8_t *data() { return buffer; }
	inline const uint8_t *data() const { return buffer; }
	inline size_t size() const { return _size; }
	inline void resize(size_t newSize)
	{
		if (_capacity < newSize) {
			reserve(newSize);
		}
		_size = newSize;
	}
	inline size_t capacity() const { return _capacity; }
	void reserve(size_t newCapacity)
	{
		if (newCapacity > _capacity) {
			uint32_t nc = ((_capacity - 1) | 63) + 1;
			if (nc < 64) {
				nc = 64;
			}
			while (nc < newCapacity) {
				nc = (nc + (nc << 1)) >> 1;
			}
			newCapacity = ((nc - 1) | 0x3F) + 1;
			auto buf = MemoryPool::Allocate(newCapacity + _offset);
			assert(buf.object);
			if (buffer == nullptr) {
				_offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
				_size = 0;
				buffer = ((uint8_t *)buf.object) + _offset;
			} else {
				uint8_t *newBuffer = ((uint8_t *)buf.object) + _offset;
				memcpy(newBuffer, buffer, _size);
				MemoryPool::Release(buffer - _offset, _capacity + _offset);
				buffer = newBuffer;
			}
			_capacity = buf.capacity - _offset;
		}
		assert(buffer);
	}

public:
	uint8_t *buffer = nullptr;
	uint32_t _size = 0;
	uint32_t _capacity = 0;
	uint32_t _offset = 0;
	uint32_t _flags = 0;
};

class ByteBufferReadable
{
public:
	ByteBufferReadable() { storage = nullptr; }
	ByteBufferReadable(ByteBufferWritable &&buffer)
	{
		if (buffer.buffer == nullptr) {
			storage = nullptr;
			return;
		}
		storage = (ByteBufferStorageHeader *)(buffer.buffer - buffer._offset);

		storage->size = buffer._size;
		storage->offset = buffer._offset;
		storage->capacity = buffer._capacity;
		storage->flags = buffer._flags;
		storage->refCounter = 1;

		buffer._size = 0;
		buffer._offset = 0;
		buffer._capacity = 0;
		buffer._flags = 0;
		buffer.buffer = nullptr;
	}

	ByteBufferReadable(ByteBufferReadable &&o)
	{
		storage = o.storage;
		o.storage = nullptr;
	}
	ByteBufferReadable(ByteBufferReadable &o)
	{
		storage = o.storage;
		if (storage) {
			storage->ref();
		}
	}
	ByteBufferReadable(const ByteBufferReadable &o)
	{
		storage = o.storage;
		if (storage) {
			storage->ref();
		}
	}

	~ByteBufferReadable()
	{
		if (storage) {
			storage->unref();
			storage = nullptr;
		}
	}

	ByteBufferReadable &operator=(ByteBufferReadable &&o)
	{
		if (storage) {
			storage->unref();
		}
		if (o.storage) {
			storage = o.storage;
		}
		o.storage = nullptr;
		return *this;
	}
	ByteBufferReadable &operator=(ByteBufferReadable &o)
	{
		if (storage) {
			storage->unref();
		}
		if (o.storage) {
			storage = o.storage;
			storage->ref();
		}
		return *this;
	}
	ByteBufferReadable &operator=(const ByteBufferReadable &o)
	{
		if (storage) {
			storage->unref();
		}
		if (o.storage) {
			storage = o.storage;
			storage->ref();
		}
		return *this;
	}

	const uint8_t *data() const
	{
		if (storage) {
			return storage->data();
		}
		return nullptr;
	}

	uint32_t size() const
	{
		if (storage) {
			return storage->size;
		}
		return 0;
	}

	uint32_t capacity() const
	{
		if (storage) {
			return storage->capacity;
		}
		return 0;
	}

	ByteBufferWritable TryRecycle()
	{
		if (storage) {
			if (storage->refCounter.load() == 1) {
				ByteBufferWritable ret;
				ret._size = 0;
				ret._flags = 0;
				ret._offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
				ret._capacity = storage->offset + storage->capacity - ret._offset;
				ret.buffer = ((uint8_t *)storage) + ret._offset;
				storage = nullptr;
				return ret;
			}
		}
		return {};
	}

	friend struct SendFrameStruct;

public:
	ByteBufferStorageHeader *storage;
};
} // namespace icon7

#endif
