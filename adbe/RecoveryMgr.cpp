#include "RecoveryMgr.h"

RecoveryMgr::RecoveryMgr(Transaction* tx, int txnum, LogMgr* lm, BufferMgr* bm) {
	this->tx = tx;
	this->txnum = txnum;
	this->lm = lm;
	this->bm = bm;
	//дһ������ʼ��־
	StartRecord::writeToLog(lm, txnum);
}

void RecoveryMgr::commit() {
	bm->flushAll(txnum);
	//дһ�������ύ��־
	int lsn = CommitRecord::writeToLog(lm, txnum);
	lm->flush(lsn);
}

void RecoveryMgr::rollback() {
	doRollback();
	bm->flushAll(txnum);
	//дһ������ع���־
	int lsn = RollbackRecord::writeToLog(lm, txnum);
	lm->flush(lsn);
}

void RecoveryMgr::recover() {
	doRecover();
	bm->flushAll(txnum);
	//дһ������ָ���־
	int lsn = CheckpointRecord::writeToLog(lm);
	lm->flush(lsn);
}

//����lsn
int RecoveryMgr::setInt(Buffer* buff, int offset, int newval) {
	int oldval = buff->contents()->getInt(offset);
	BlockId* blk = buff->block();
	//дһ��SetInt��־
	return SetIntRecord::writeToLog(lm, txnum, blk, offset, oldval);
}

//����lsn
int RecoveryMgr::setString(Buffer* buff, int offset, const std::string& newval) {
	std::string oldval = buff->contents()->getString(offset);
	BlockId* blk = buff->block();
	//
	return SetStringRecord::writeToLog(lm, txnum, blk, offset, oldval);
}

//�ع�����
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

//���ݿ�ָ�
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