#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include "RecoveryMgr.h"
#include "ConcurrencyMgr.h"
#include "BufferMgr.h"
#include "FileMgr.h"
#include "BufferList.h"

#include "mysql/thread_pool_priv.h"

class RecoveryMgr;

enum TransStatus {
	TX_XLOCK_FAILED = -1,
	TX_SLOCK_FAILED = -1,
	TX_SUCC = 0
};

class Transaction {
public:
	static void init();
	static void deinit();
	static int nextTxNumber();

	Transaction(FileMgr* fm, LogMgr* lm, BufferMgr* bm, LockTable* locktbl);
	~Transaction();
	
	void commit();
	void rollback();
	void recover();

	bool pin(BlockId* blk);
	void unpin(BlockId* blk);

	TransStatus getInt(BlockId* blk, int offset,int& out);
	TransStatus getString(BlockId* blk, int offset,std::string& out);

	//okToLog 表示是否写入日志
	TransStatus setInt(BlockId* blk, int offset, int val, bool okToLog);
	TransStatus setString(BlockId* blk, int offset, const std::string& val, bool okToLog);

	//返回文件中的块数
	TransStatus size(const std::string& filename,int& out);
	TransStatus append(const std::string& filename,BlockId* & out);

	int blockSize() {
		return fm->blockSize();
	}

	int availableBuffs() {
		return bm->available();
	}
private:
	static mysql_mutex_t transNextTxMutex;
	static int nextTxNum;
	const static int END_OF_FILE = -1;

	RecoveryMgr*    recoveryMgr;
	ConcurrencyMgr* concurMgr;
	BufferMgr* bm;
	FileMgr* fm;
	int txnum;
	BufferList* mybuffers;
};

#endif // !__TRANSACTION_H__
