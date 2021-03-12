#ifndef __VIEW_MGR_H__
#define __VIEW_MGR_H__

#include "TableMgr.h"

class ViewMgr {
public:
	ViewMgr(bool isNew, TableMgr* tblMgr, Transaction* tx);

	void createView(const std::string& vname, const std::string& vdef, Transaction* tx);

	std::string getViewDef(const std::string& vname, Transaction* tx);
private:
	// the max chars in a view definition.
	const static int MAX_VIEWDEF = 100;

	TableMgr* tblMgr;
};

#endif // !__VIEW_MGR_H__