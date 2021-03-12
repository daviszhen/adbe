#ifndef __INDEX_MGR_H__
#define __INDEX_MGR_H__

#include "TableMgr.h"
#include "StatMgr.h"

#include <unordered_map>
#include <string>
#include "Index.h"

class IndexInfo {
public:
	IndexInfo(const std::string& idxname, const std::string& fldname,
		Schema& tblSchema,
		Transaction* tx, StatInfo& si);
	
	Index* open();

	int blocksAccessed();

	int recordsOutput();

	int distinctValues(const std::string& fname);
private:
	Layout* createIdxLayout();

private:
	std::string idxname, fldname;
	Transaction* tx;
	Schema tblSchema;
	Layout* idxLayout;
	StatInfo si;
};

class IndexMgr {
public:
	IndexMgr(bool isnew, TableMgr* tblmgr, StatMgr* statmgr, Transaction* tx);

	void createIndex(const std::string& idxname, 
		const std::string& tblname, 
		const std::string& fldname, Transaction* tx);

	std::unordered_map<std::string, IndexInfo*> getIndexInfo(const std::string& tblname, Transaction* tx);

private:
	Layout* layout;
	TableMgr* tblmgr;
	StatMgr* statmgr;

};

#endif // !__INDEX_MGR_H__
