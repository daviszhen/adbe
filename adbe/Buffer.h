#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "FileMgr.h"
#include "LogMgr.h"
#include "page.h"
#include "blockid.h"

class Buffer {
public:
	Buffer(FileMgr* fm, LogMgr* lm) {
		this->fm = fm;
		this->lm = lm;
		this->contentsPage = Page::New(fm->blockSize());
		this->blk = nullptr;
		this->pins = 0;
		this->txnum = -1;
		this->lsn = -1;
	}

	Page* contents() {
		return contentsPage;
	}

	BlockId* block() {
		return blk;
	}

	//��������ź�lsn
	void setModified(int txnum, int lsn) {
		this->txnum = txnum;
		if (lsn >= 0)
			this->lsn = lsn;
	}

	bool isPinned() {
		return pins > 0;
	}

	int modifyingTx() {
		return txnum;
	}

	//��������ݶ���ҳ��ҳ��֮ǰ�����ݣ�д�����
	void assignToBlock(BlockId* b) {
		flush();
		blk = b;
		fm->read(*blk, *contentsPage);
		pins = 0;
	}

	//���ҳ���ˣ�Ҫд�����
	void flush() {
		if (txnum >= 0) {
			lm->flush(lsn);
			fm->write(*blk, *contentsPage);
			txnum = -1;
		}
	}

	void pin() {
		pins++;
	}

	void unpin() {
		pins--;
	}
private:
	FileMgr* fm;
	LogMgr* lm;
	Page* contentsPage;
	BlockId* blk;
	int pins;
	int txnum;
	int lsn;
};

#endif