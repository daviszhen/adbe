#ifndef __LOG_ITERATOR_H__
#define __LOG_ITERATOR_H__
#include "FileMgr.h"
#include "blockid.h"
#include "page.h"

class LogIterator {
public:
	LogIterator(FileMgr* fm, const BlockId* blk) {
		this->fm = fm;
		this->blk = BlockId::New(blk->fileName(),blk->number());
		this->p = Page::New(fm->blockSize());
		moveToBlock(this->blk);
	}

	~LogIterator() {
		delete blk;
		delete p;
	}

	//�Ƿ�����־��¼
	bool hasNext() {
		//��־��¼Ҫô�ڵ�ǰ�飬������һ����
		return currentpos < fm->blockSize() || blk->number() > 0;
	}

	//������һ����־��¼
	std::unique_ptr<ByteBuffer> next() {
		if (currentpos == fm->blockSize()) {//��ǰ������ˣ��л�����һ����¼
			BlockId* newBlk = new BlockId(blk->fileName(), blk->number() - 1);
			switchBlockId(newBlk);
			moveToBlock(blk);
		}
		std::unique_ptr<ByteBuffer> rec = p->getBytes(currentpos);
		currentpos += sizeof(int) + rec->getCapacity();
		return rec;
	}
private:
	//����������λ��ָ����
	void moveToBlock(BlockId* blk) {
		fm->read(*blk, *p);
		boundary = p->getInt(0);
		currentpos = boundary;
	}

	void switchBlockId (BlockId * newBlk) {
		delete blk;
		blk = newBlk;
	}
private:
	FileMgr* fm;
	BlockId* blk;
	Page* p;
	int currentpos;
	int boundary;
};
#endif // !__LOG_ITERATOR_H__
