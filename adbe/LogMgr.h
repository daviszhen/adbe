#ifndef __LOG_MRG_H__
#define __LOG_MRG_H__

#include "FileMgr.h"
#include "page.h"
#include "blockid.h"
#include "LogIterator.h"

class LogMgr
{
public:
	LogMgr(FileMgr* fm,const std::string& logfile) {
		this->fm = fm;
		this->logfile = logfile;
		this->latestLSN = 0;
		this->lastSavedLSN = 0;
		logpage = Page::New(fm->blockSize());
		int logsize = fm->length(logfile);
		if (logsize == 0) {
			currentblk = appendNewBlock();;
		}
		else {
			currentblk = BlockId::New(logfile, logsize - 1);
			fm->read(*currentblk, *logpage);
		}
		mysql_mutex_init(NULL, &logMgrMutex, MY_MUTEX_INIT_FAST);
	}
	static LogMgr* New(FileMgr* fm, const std::string& logfile) {
		return new LogMgr(fm, logfile);
	}
	~LogMgr() {
		delete logpage;
		delete currentblk;
		mysql_mutex_destroy(&logMgrMutex);
	}

	//指定lsn之前的日志记录都写入日志文件
	void flush(int lsn) {
		if (lsn >= lastSavedLSN) {
			flush();
		}
	}

	//当前块的迭代器
	LogIterator* iterator() {
		flush();
		return new LogIterator(fm, currentblk);
	}

	//日志记录在页中，从右向左放置，是以反向链表的形式存储的。
	//每个日志记录的后面都放置一个整数，指向前一个记录的位置?
	int append(ByteBuffer* logrec) {
		mysql_mutex_lock(&logMgrMutex);
		assert(logrec != nullptr);
		//空闲空间的结尾处
		int boundary = logpage->getInt(0);
		int recsize = logrec->getCapacity();
		//缓冲区内容长度+前一个记录位置偏移?
		int bytesneeded = recsize + sizeof(int);
		if (boundary  < bytesneeded + sizeof(int)) {
			flush();
			currentblk = appendNewBlock();
			boundary = logpage->getInt(0);
		}
		int recpos = boundary - bytesneeded;

		logpage->setBytes(recpos, logrec->getBuffer(),logrec->getCapacity());
		logpage->setInt(0, recpos); 
		latestLSN += 1;
		mysql_mutex_unlock(&logMgrMutex);
		return latestLSN;
	}
private:
	//在logfile尾部写入新块，并在块的头部写入日志记录的开始位置.
	BlockId* appendNewBlock() {
		BlockId* blk = fm->append(logfile);
		logpage->setInt(0, fm->blockSize());
		fm->write(*blk, *logpage);
		return blk;
	}

	//将logpage中内容，写入指定块
	void flush() {
		fm->write(*currentblk, *logpage);
		lastSavedLSN = latestLSN;
	}

private:
	mysql_mutex_t logMgrMutex;
	FileMgr* fm;
	std::string logfile;
	Page* logpage;
	BlockId* currentblk;
	int latestLSN;
	int lastSavedLSN;
};

#endif // !__LOG_MRG_H__
