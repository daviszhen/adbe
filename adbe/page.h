#ifndef __PAGE_H__
#define __PAGE_H__

#include "bytebuffer.h"
#include <string>
#include <memory>

//Pageӵ���ڲ��������Ŀ���Ȩ����Ҫ�����ͷ��ڲ�������
class Page {
public:
	Page(int blocksize) {
		bb = ByteBuffer::New(blocksize);
	}

	Page(char* b,int blen) {
		bb = ByteBuffer::New(b, blen);
	}

	Page(ByteBuffer* bbuf) {
		assert(bbuf != nullptr);
		bb = bbuf;
	}

	static Page* New(int blocksize) {
		return new Page(blocksize);
	}

	static Page* New(char* b, int blen) {
		return new Page(b, blen);
	}

	static Page* New(ByteBuffer* bbuf) {
		return new Page(bbuf);
	}

	~Page() {
		delete bb;
	}

	int getInt(int offset) {
		return bb->getInt(offset);
	}

	void setInt(int offset, int n) {
		bb->putInt(offset, n);
	}

	std::unique_ptr<ByteBuffer> getBytes(int offset) {
		int length = bb->getInt(offset);
		assert(length > 0);
		ByteBuffer* tbb = ByteBuffer::New(length);
		assert(tbb != nullptr);
		bb->get(offset+sizeof(int),tbb->getBuffer(), length);
		return std::unique_ptr<ByteBuffer>(tbb);
	}

	void setBytes(int offset, const char* b, int blen) {
		bb->putInt(offset, blen);
		bb->put(offset + sizeof(int), b, blen);
	}

	std::string getString(int offset) {
		std::unique_ptr<ByteBuffer> b= getBytes(offset);
		return std::string(b->getBuffer(),b->getCapacity());
	}

	void setString(int offset, const std::string& s) {
		setBytes(offset, s.c_str(), static_cast<int>(s.size()));
	}

	//strlen�����ַ�������Ҫ�Ĵ洢�ռ���
	static int maxLength(int strlen) {
		return sizeof(int) + strlen;
	}

	ByteBuffer* contents() {
		return bb;
	}

	//�ͷŻ���������Ȩ
	//ȡ��ҳ�еĻ�������ҳ�л�����Ϊ�ա�
	ByteBuffer* extractBuffer() {
		ByteBuffer* ret = bb;
		bb = nullptr;
		return ret;
	}

	const ByteBuffer* contents() const{
		return bb;
	}
private:
	ByteBuffer* bb;//������
};

#endif // !__PAGE_H__
