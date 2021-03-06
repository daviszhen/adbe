#include "RecoveryMgr.h"

RecoveryMgr::RecoveryMgr(Transaction* tx, int txnum, LogMgr* lm, BufferMgr* bm) {
	this->tx = tx;
	this->txnum = txnum;
	this->lm = lm;
	this->bm = bm;
	//写一个事务开始日志
	StartRecord::writeToLog(lm, txnum);
}

void RecoveryMgr::commit() {
	bm->flushAll(txnum);
	//写一个事务提交日志
	int lsn = CommitRecord::writeToLog(lm, txnum);
	lm->flush(lsn);
}

void RecoveryMgr::rollback() {
	doRollback();
	bm->flushAll(txnum);
	//写一个事务回滚日志
	int lsn = RollbackRecord::writeToLog(lm, txnum);
	lm->flush(lsn);
}

void RecoveryMgr::recover() {
	doRecover();
	bm->flushAll(txnum);
	//写一个事务恢复日志
	int lsn = CheckpointRecord::writeToLog(lm);
	lm->flush(lsn);
}

//返回lsn
int RecoveryMgr::setInt(Buffer* buff, int offset, int newval) {
	int oldval = buff->contents()->getInt(offset);
	BlockId* blk = buff->block();
	//写一个SetInt日志
	return SetIntRecord::writeToLog(lm, txnum, blk, offset, oldval);
}

//返回lsn
int RecoveryMgr::setString(Buffer* buff, int offset, const std::string& newval) {
	std::string oldval = buff->contents()->getString(offset);
	BlockId* blk = buff->block();
	//
	return SetStringRecord::writeToLog(lm, txnum, blk, offset, oldval);
}

//回归事务
void RecoveryMgr::doRollback() {
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
void RecoveryMgr::doRecover() {
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