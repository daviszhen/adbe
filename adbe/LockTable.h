#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include "blockid.h"
#include "mysql/thread_pool_priv.h"

#include <ctime>
#include <unordered_map>

class LockTable {
public:
	LockTable(){
		mysql_mutex_init(NULL, &lockTableMutex, MY_MUTEX_INIT_FAST);
		mysql_cond_init(NULL, &lockTableMutex_cond);
	}
	~LockTable(){
		mysql_mutex_destroy(&lockTableMutex);
		mysql_cond_destroy(&lockTableMutex_cond);
	}

	bool sLock(BlockId* blk) {
		mysql_mutex_lock(&localTableMutex);
		std::time_t timestamp = std::time(nullptr);
		struct timespec ts { MAX_TIME, 0 };
		while (hasXlock(blk) && !waitingTooLong(timestamp)) {
			mysql_cond_timedwait(&lockTableMutex_cond, &lockTableMutex, &ts);
		}
		if (hasXlock(blk)) {
			mysql_mutex_unlock(&localTableMutex);
			return false;
		}
		int val = getLockVal(blk);  // will not be negative
		locks[blk] =  val + 1;
		mysql_mutex_unlock(&localTableMutex);
		return true;
	}

	bool xLock(BlockId* blk) {
		mysql_mutex_lock(&localTableMutex);
		std::time_t timestamp = std::time(nullptr);
		struct timespec ts { MAX_TIME, 0 };
		while (hasOtherSLocks(blk) && !waitingTooLong(timestamp)) {
			mysql_cond_timedwait(&lockTableMutex_cond, &lockTableMutex, &ts);
		}
		if (hasOtherSLocks(blk)) {
			mysql_mutex_unlock(&localTableMutex);
			return false;
		}
		locks[blk] = -1;
		mysql_mutex_unlock(&localTableMutex);
		return true;
	}

	void unlock(BlockId* blk) {
		mysql_mutex_lock(&localTableMutex);
		int val = getLockVal(blk);
		if (val > 1)
			locks[blk] = val - 1;
		else {
			locks.erase(blk);
		}
		mysql_mutex_unlock(&localTableMutex);
	}

private:
	bool hasXlock(BlockId* blk) {
		return getLockVal(blk) < 0;
	}

	bool hasOtherSLocks(BlockId* blk) {
		return getLockVal(blk) > 1;
	}

	bool waitingTooLong(long starttime) {
		return std::difftime(std::time(nullptr), starttime) > MAX_TIME;
	}

	int getLockVal(BlockId* blk) {
		int ival = locks.count(blk);
		return ival;
	}
private:
	const static int MAX_TIME = 1;//1second
	std::unordered_map<BlockId*, int> locks;
	mysql_mutex_t lockTableMutex;
	mysql_cond_t lockTableMutex_cond;
};

#endif // !__LOCK_TABLE_H__

