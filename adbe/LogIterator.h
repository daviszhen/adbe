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

	//是否还有日志记录
	bool hasNext() {
		//日志记录要么在当前块，或者下一个块
		return currentpos < fm->blockSize() || blk->number() > 0;
	}

	//返回下一个日志记录
	std::unique_ptr<ByteBuffer> next() {
		if (currentpos == fm->blockSize()) {//当前块结束了，切换到下一个记录
			BlockId* newBlk = new BlockId(blk->fileName(), blk->number() - 1);
			switchBlockId(newBlk);
			moveToBlock(blk);
		}
		std::unique_ptr<ByteBuffer> rec = p->getBytes(currentpos);
		currentpos += sizeof(int) + rec->getCapacity();
		return rec;
	}
private:
	//将迭代器定位到指定块
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
