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

#include "../../bitscpp/include/bitscpp/ByteReader_v2.hpp"

#include "Flags.hpp"
#include "MemoryPool.hpp"

#include "FramingProtocol.hpp"

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

void PrintBufferMetadata(ByteBufferStorageHeader *buffer);

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

	inline void prepend(const uint8_t *src, const uint32_t bytes)
	{
		assert((buffer && _capacity) || ((!buffer) && (!_capacity)));
		if (_offset < sizeof(ByteBufferStorageHeader) + bytes) {
			ByteBufferWritable buf(bytes + _capacity);
			if (src) {
				buf.append(src, bytes);
			} else {
				buf._size += bytes;
			}
			if (_size) {
				buf.append(buffer, _size);
			}
			*this = std::move(buf);
			return;
		} else if (buffer == nullptr) {
			if (src) {
				append(src, bytes);
			} else {
				resize(bytes);
			}
			return;
		}
		buffer -= bytes;
		_offset -= bytes;
		_size += bytes;
		_capacity += bytes;
		if (src) {
			memcpy(buffer, src, bytes);
		}
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

	void reserve(const size_t newCapacity)
	{
		assert(_size <= _capacity);
		if (newCapacity > _capacity) {
			if (buffer == nullptr) {
				_offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
				uint32_t nc = ((_offset + newCapacity - 1) | 0x3F) + 1;
				if (nc < 64) {
					nc = 64;
				}
				assert(nc >= newCapacity + _offset);
				auto buf = MemoryPool::Allocate(nc);
				assert(buf.object);
				assert(buf.capacity >= newCapacity + _offset);
				_size = 0;
				buffer = ((uint8_t *)buf.object) + _offset;
				_capacity = buf.capacity - _offset;
			} else {
				uint32_t nc = ((_offset + _capacity - 1) | 0x3F) + 1;
				if (nc < 64) {
					nc = 64;
				}
				while (nc < newCapacity + _offset) {
					nc = (nc + (nc << 1)) >> 1;
				}
				nc = ((nc - 1) | 0x3F) + 1;
				assert(nc >= newCapacity + _offset);

				ByteBufferWritable buf(nc - _offset);
				buf.append(buffer, _size);
				buf._flags = _flags;
				*this = std::move(buf);
			}
			assert(buffer);
		} else {
			assert(buffer);
			assert(_capacity >= newCapacity);
		}
	}

	friend class ByteBufferReadable;
protected:
	uint8_t *buffer = nullptr;
	uint32_t _size = 0;
	uint32_t _capacity = 0;
	uint32_t _offset = 0;

public:
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
		
		PrintBufferMetadata(storage);
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
				const uint32_t cap = storage->offset + storage->capacity;
				uint8_t *const ptr = (uint8_t *)storage;
				ByteBufferWritable ret;
				ret._size = 0;
				ret._flags = 0;
				ret._offset = ByteBufferStorageHeader::INITIAL_DATA_OFFSET;
				ret._capacity = cap - ret._offset;
				ret.buffer = ptr + ret._offset;
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
