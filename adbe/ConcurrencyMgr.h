#ifndef __CONCURRENCY_MGR_H__
#define __CONCURRENCY_MGR_H__

#include "LockTable.h"
#include "blockid.h"

#include <unordered_map>
#include <string>

//ÿ��������һ��ConcurrencyMgr
class ConcurrencyMgr {
public:
	ConcurrencyMgr(){}
	~ConcurrencyMgr(){}

	bool sLock(BlockId* blk) {
		if (locks.count(blk) == 0) {
			bool succ = locktbl->sLock(blk);
			if (succ) {
				locks[blk]= "S";
				return true;
			}
		}
		return false;
	}

	void release() {
		for (auto pr : locks) {
			locktbl->unlock(pr.first);
		}
		locks.clear();
	}
private:
	bool hasXLock(BlockId* blk) {
		return locks.count(blk) && locks[blk] == "X";
	}
private:
	//���е�������ͬһ��LockTable
	static LockTable* locktbl;
	std::unordered_map<BlockId*, std::string> locks;
};
#endif // !__CONCURRENCY_MGR_H__
