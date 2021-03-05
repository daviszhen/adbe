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

	//ָ��lsn֮ǰ����־��¼��д����־�ļ�
	void flush(int lsn) {
		if (lsn >= lastSavedLSN) {
			flush();
		}
	}

	//��ǰ��ĵ�����
	LogIterator* iterator() {
		flush();
		return new LogIterator(fm, currentblk);
	}

	//��־��¼��ҳ�У�����������ã����Է����������ʽ�洢�ġ�
	//ÿ����־��¼�ĺ��涼����һ��������ָ��ǰһ����¼��λ��?
	int append(ByteBuffer* logrec) {
		mysql_mutex_lock(&logMgrMutex);
		assert(logrec != nullptr);
		//���пռ�Ľ�β��
		int boundary = logpage->getInt(0);
		int recsize = logrec->getCapacity();
		//���������ݳ���+ǰһ����¼λ��ƫ��?
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
	//��logfileβ��д���¿飬���ڿ��ͷ��д����־��¼�Ŀ�ʼλ��.
	BlockId* appendNewBlock() {
		BlockId* blk = fm->append(logfile);
		logpage->setInt(0, fm->blockSize());
		fm->write(*blk, *logpage);
		return blk;
	}

	//��logpage�����ݣ�д��ָ����
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
