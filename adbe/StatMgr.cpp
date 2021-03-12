#include "StatMgr.h"
#include "Scan.h"

StatMgr::StatMgr(TableMgr* tblMgr, Transaction* tx) {
	mysql_mutex_init(NULL, &statMgrMutex, MY_MUTEX_INIT_FAST);
	this->tblMgr = tblMgr;
	refreshStatistics(tx);
}

StatMgr::~StatMgr() {
	mysql_mutex_destroy(&statMgrMutex);
}

StatInfo StatMgr::getStatInfo(const std::string& tblname, Layout* layout, Transaction* tx) {
	StatInfo ret;
	mysql_mutex_lock(&statMgrMutex);
	numcalls++;
	if (numcalls > 100)
		refreshStatistics(tx);
	if (tablestats.count(tblname)) {
		tablestats[tblname] = calcTableStats(tblname, layout, tx);
	}
	ret = tablestats[tblname];
	mysql_mutex_unlock(&statMgrMutex);
	return ret;
}

void StatMgr::refreshStatistics(Transaction* tx) {
	mysql_mutex_lock(&statMgrMutex);
	numcalls = 0;
	Layout* tcatlayout = tblMgr->getLayout("tblcat", tx);
	TableScan tcat(tx, "tblcat", tcatlayout);
	while (tcat.next()) {
		std::string tblname = tcat.getString("tblname");
		Layout* layout = tblMgr->getLayout(tblname, tx);
		StatInfo si = calcTableStats(tblname, layout, tx);
		tablestats[tblname]=si;
	}
	tcat.close();
	mysql_mutex_unlock(&statMgrMutex);
}

StatInfo StatMgr::calcTableStats(const std::string& tblname, Layout* layout, Transaction* tx){
	int numRecs = 0;
	 int numblocks = 0;
	 TableScan ts(tx, tblname, layout);
	 while (ts.next()) {
		 numRecs++;
		 numblocks = ts.getRid().blockNumber() + 1;
	 }
	 ts.close();
	 return StatInfo(numblocks, numRecs);
}