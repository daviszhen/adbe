#ifndef __BUFFER_MGR_H__
#define __BUFFER_MGR_H__

#include "Buffer.h"
#include "ha_example.h"
#include "mysql/thread_pool_priv.h"

#include <ctime>
#include <vector>

class BufferMgr {
public:
	BufferMgr(FileMgr* fm, LogMgr* lm, int numbuffs) {
		bufferpool.resize(numbuffs);
		numAvailable = numbuffs;
		for (int i = 0; i < numbuffs; i++)
			bufferpool[i] = new Buffer(fm, lm);
		mysql_mutex_init(NULL, &bufferMgrMutex, MY_MUTEX_INIT_FAST);
		mysql_cond_init(NULL,&bufferMgrMutex_cond);
	}

	static BufferMgr* New(FileMgr* fm, LogMgr* lm, int numbuffs) {
		return new BufferMgr(fm, lm, numbuffs);
	}

	~BufferMgr() {
		for (int i = 0; i < this->numAvailable; i++) {
			delete bufferpool[i];
			bufferpool[i] = nullptr;
		}
		mysql_mutex_destroy(&bufferMgrMutex);
		mysql_cond_destroy(&bufferMgrMutex_cond);
	}

	int available() {
		mysql_mutex_lock(&bufferMgrMutex);
		int count = numAvailable;
		mysql_mutex_unlock(&bufferMgrMutex);
		return count;
	}

	//刷新指定事务的脏页到磁盘
	void flushAll(int txnum) {
		mysql_mutex_lock(&bufferMgrMutex);
		for (Buffer* buff : bufferpool)
			if (buff->modifyingTx() == txnum)
				buff->flush();
		mysql_mutex_unlock(&bufferMgrMutex);
	}

	//使buff空闲
	void unpin(Buffer* buff) {
		mysql_mutex_lock(&bufferMgrMutex);
		buff->unpin();
		if (!buff->isPinned()) {
			numAvailable++;
		}
		mysql_mutex_unlock(&bufferMgrMutex);
	}

	//超时还未取得缓冲区时，返回NULL
	Buffer* pin(BlockId* blk) {
		mysql_mutex_lock(&bufferMgrMutex);
		Buffer* buff = tryToPin(blk);
		std::time_t timestamp = std::time(nullptr);
		struct timespec ts {MAX_TIME,0};
		while (buff == nullptr && !waitingTooLong(timestamp)) {
			//sql_print_information("cond wait \n");
			mysql_cond_timedwait(&bufferMgrMutex_cond, &bufferMgrMutex,&ts);
			//sql_print_information("cond timeout \n");
			buff = tryToPin(blk);
		}
		
		mysql_mutex_unlock(&bufferMgrMutex);
		return buff;
	}

private:
	bool waitingTooLong(std::time_t starttime) {
		return std::difftime(std::time(nullptr), starttime) > MAX_TIME;
	}

	Buffer* tryToPin(BlockId* blk) {
		Buffer* buff = findExistingBuffer(blk);
		if (buff == nullptr) {
			buff = chooseUnpinnedBuffer();
			if (buff == nullptr)
				return nullptr;
			buff->assignToBlock(blk);
		}
		if (!buff->isPinned())
			numAvailable--;
		buff->pin();
		return buff;
	}

	Buffer* findExistingBuffer(BlockId* blk) {
		for (Buffer* buff : bufferpool) {
			BlockId* b = buff->block();
			if (b != nullptr && *b == *blk)
				return buff;
		}
		return nullptr;
	}

	Buffer* chooseUnpinnedBuffer() {
		for (Buffer* buff : bufferpool)
			if (!buff->isPinned())
				return buff;
		return nullptr;
	}
private:
	mysql_mutex_t bufferMgrMutex;
	mysql_cond_t bufferMgrMutex_cond;
	std::vector<Buffer*> bufferpool;
	int numAvailable;

	const static long MAX_TIME = 1; // 1 seconds
};

#endif // !__BUFFER_MGR_H__
