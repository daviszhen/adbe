#ifndef __TABLE_MGR_H__
#define __TABLE_MGR_H__

#include "Transaction.h"
#include "Layout.h"

class TableMgr {
public:
	TableMgr(bool isNew, Transaction* tx);

	void createTable(const std::string& tblname, Schema* sch, Transaction* tx);

	Layout* getLayout(const std::string& tblname, Transaction* tx);
public:
	const static int MAX_NAME = 16;
private:
	Layout* tcatLayout, *fcatLayout;
};

#endif