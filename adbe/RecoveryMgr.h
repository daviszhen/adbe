#ifndef __RECOVERY_MGR_H__
#define __RECOVERY_MGR_H__

#include "LogMgr.h"
#include "BufferMgr.h"
#include "LogRecord.h"
#include "Buffer.h"

#include <set>

class Transaction;

//ÿ���������Լ���RecoveryMgr
class RecoveryMgr {
public:
	RecoveryMgr(Transaction* tx, int txnum, LogMgr* lm, BufferMgr* bm);

	void commit();

	void rollback();

	void recover();

	//����lsn
	int setInt(Buffer* buff, int offset, int newval);

	//����lsn
	int setString(Buffer* buff, int offset, const std::string& newval);
private:
	//�ع�����
	void doRollback();

	//���ݿ�ָ�
	void doRecover();
private:
	LogMgr* lm;
	BufferMgr* bm;
	Transaction* tx;
	int txnum;
};

#endif // !__RECOVERY_MGR_H__
