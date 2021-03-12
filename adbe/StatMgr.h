#ifndef __STAT_MGR_H__
#define __STAT_MGR_H__

#include <unordered_map>

#include "TableMgr.h"
#include "mysql/thread_pool_priv.h"

class StatInfo {
public:
	StatInfo(int numblocks = -1, int numrecs = -1) {
		this->numBlocks = numblocks;
		this-> numRecs = numrecs;
	}

	int blocksAccessed() {
		return numBlocks;
	}

	int recordsOutput() {
		return numRecs;
	}

	int distinctValues(const std::string& fldname) {
		return 1 + (numRecs / 3);
	}
private:
	int numBlocks;
	int numRecs;
};

class StatMgr {
public:
	StatMgr(TableMgr* tblMgr, Transaction* tx);
	~StatMgr();
	StatInfo getStatInfo(const std::string& tblname, Layout* layout, Transaction* tx);
public:
	void refreshStatistics(Transaction* tx);

	StatInfo calcTableStats(const std::string& tblname, Layout* layout, Transaction* tx);
private:
	mysql_mutex_t statMgrMutex;
	TableMgr* tblMgr;
	std::unordered_map<std::string, StatInfo> tablestats;
	int numcalls;
};
#endif // !__STAT_MGR_H__
