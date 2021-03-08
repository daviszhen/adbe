#ifndef __BYTE_BUFFER_H__
#define __BYTE_BUFFER_H__

#include <cassert>
#include <cstring>

//ByteBuffer拥有内部缓冲区的控制权，需要主动释放内存
class ByteBuffer {
public:
	ByteBuffer(int cap) {
		assert(cap >= 0);
		capacity = cap;
		if (cap == 0) {
			buffer = new char[1];
		}
		else {
			buffer = new char[cap];
		}
	}
	ByteBuffer(char* buf, int cap) {
		assert(buf != nullptr && cap > 0);
		capacity = cap;
		buffer = buf;
	}
	~ByteBuffer() {
		delete[] buffer;
	}
	static ByteBuffer* New(int cap) {
		return new ByteBuffer(cap);
	}

	static ByteBuffer* New(char* buf,int cap) {
		return new ByteBuffer(buf,cap);
	}

	int getInt(int index) {
		indexIsValid(index);
		int* ptr = reinterpret_cast<int*>(buffer + index);
		return *ptr;
	}

	void putInt(int index, int value) {
		indexIsValid(index);
		int* ptr = reinterpret_cast<int*>(buffer + index);
		*ptr = value;
	}

	void get(int index, char* buf,int bufLen) {
		indexIsValid(index);
		assert(buf != nullptr);
		assert(capacity - index >= bufLen);
		memcpy(buf, buffer + index, bufLen);
	}

	void put(int index,const char* buf, int bufLen) {
		indexIsValid(index);
		assert(buf != nullptr);
		assert(capacity - index >= bufLen);
		memcpy(buffer + index, buf, bufLen);
	}

	char* getBuffer()const {
		return buffer;
	}

	int getCapacity()const {
		return capacity;
	}
private:
	void indexIsValid(int index) {
		assert(index >= 0 && index < capacity);
	}
private:
	int capacity;
	char* buffer;
};

#endif
