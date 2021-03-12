#ifndef __TABLE_PLAN_H__
#define __TABLE_PLAN_H__

#include "Scan.h"
#include "Plan.h"
#include "MetadataMgr.h"

class TablePlan : public Plan {
public:
	TablePlan(Transaction* tx, const std::string& tblname, MetadataMgr* md) {
		this->tblname = tblname;
		this->tx = tx;
		layout = md->getLayout(tblname, tx);
		si = md->getStatInfo(tblname, layout, tx);
	}

	Scan* open() {
		return new TableScan(tx, tblname, layout);
	}

	int blocksAccessed() {
		return si.blocksAccessed();
	}

	int recordsOutput() {
		return si.recordsOutput();
	}

	int distinctValues(const std::string& fldname) {
		return si.distinctValues(fldname);
	}

	Schema schema() {
		return layout->schema();
	}
private:
	std::string tblname;
	Transaction* tx;
	Layout* layout;
	StatInfo si;
};
#endif // !__TABLE_PLAN_H__
