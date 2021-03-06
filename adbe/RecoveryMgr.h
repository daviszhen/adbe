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
	RecoveryMgr(Transaction* tx, int txnum, LogMgr* lm, BufferMgr* bm);

	void commit();

	void rollback();

	void recover();

	//返回lsn
	int setInt(Buffer* buff, int offset, int newval);

	//返回lsn
	int setString(Buffer* buff, int offset, const std::string& newval);
private:
	//回归事务
	void doRollback();

	//数据库恢复
	void doRecover();
private:
	LogMgr* lm;
	BufferMgr* bm;
	Transaction* tx;
	int txnum;
};

#endif // !__RECOVERY_MGR_H__
