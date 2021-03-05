#ifndef __RECOVERY_MGR_H__
#define __RECOVERY_MGR_H__

#include "LogMgr.h"
#include "BufferMgr.h"
#include "LogRecord.h"
#include "Buffer.h"

#include <set>

class Transaction;

//每个事务有自己的RecoveryMgr
class RecoveryMgr {
public:
	RecoveryMgr(Transaction* tx, int txnum, LogMgr* lm, BufferMgr* bm) {
		this->tx = tx;
		this->txnum = txnum;
		this->lm = lm;
		this->bm = bm;
		//写一个事务开始日志
		StartRecord::writeToLog(lm, txnum);
	}

	void commit() {
		bm->flushAll(txnum);
		//写一个事务提交日志
		int lsn = CommitRecord::writeToLog(lm, txnum);
		lm->flush(lsn);
	}

	void rollback() {
		doRollback();
		bm->flushAll(txnum);
		//写一个事务回滚日志
		int lsn = RollbackRecord::writeToLog(lm, txnum);
		lm->flush(lsn);
	}

	void recover() {
		doRecover();
		bm->flushAll(txnum);
		//写一个事务恢复日志
		int lsn = CheckpointRecord::writeToLog(lm);
		lm->flush(lsn);
	}

	//返回lsn
	int setInt(Buffer* buff, int offset, int newval) {
		int oldval = buff->contents()->getInt(offset);
		BlockId* blk = buff->block();
		//写一个SetInt日志
		return SetIntRecord::writeToLog(lm, txnum, blk, offset, oldval);
	}

	//返回lsn
	int setString(Buffer* buff, int offset, const std::string& newval) {
		std::string oldval = buff->contents()->getString(offset);
		BlockId* blk = buff->block();
		//
		return SetStringRecord::writeToLog(lm, txnum, blk, offset, oldval);
	}
private:
	//回归事务
	void doRollback() {
		LogIterator* iter = lm->iterator();
		while (iter->hasNext()) {
			std::unique_ptr<ByteBuffer> bytes = iter->next();
			LogRecord* rec = LogRecord::createLogRecord(bytes.get());
			if (rec->txNumber() == txnum) {
				if (rec->op() == START)
					return;
				rec->undo(tx);
			}
		}
	}

	//数据库恢复
	void doRecover() {
		std::set<int> finishedTxs;
		LogIterator* iter = lm->iterator();
		while (iter->hasNext()) {
			std::unique_ptr<ByteBuffer> bytes = iter->next();
			LogRecord* rec = LogRecord::createLogRecord(bytes.get());
			if (rec->op() == CHECKPOINT)
				return;
			if (rec->op() == COMMIT || rec->op() == ROLLBACK)
				finishedTxs.insert(rec->txNumber());
			else if (!finishedTxs.count(rec->txNumber()))
				rec->undo(tx);
		}
	}
private:
	LogMgr* lm;
	BufferMgr* bm;
	Transaction* tx;
	int txnum;
};

#endif // !__RECOVERY_MGR_H__
