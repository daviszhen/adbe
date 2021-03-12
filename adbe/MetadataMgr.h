#ifndef __METADATA_MGR_H_
#define __METADATA_MGR_H_

#include "TableMgr.h"
#include "ViewMgr.h"
#include "StatMgr.h"
#include "IndexMgr.h"

class MetadataMgr {
public:
	MetadataMgr(bool isnew, Transaction* tx) {
		tblmgr = new TableMgr(isnew, tx);
		viewmgr = new ViewMgr(isnew, tblmgr, tx);
		statmgr = new StatMgr(tblmgr, tx);
		idxmgr = new IndexMgr(isnew, tblmgr, statmgr, tx);
	}
	~MetadataMgr() {
		if (tblmgr) {
			delete tblmgr;
			tblmgr = nullptr;
		}
		if (viewmgr) {
			delete viewmgr;
			viewmgr = nullptr;
		}
		if (statmgr) {
			delete statmgr;
			statmgr = nullptr;
		}
		if (idxmgr) {
			delete idxmgr;
			idxmgr = nullptr;
		}
	}

	void createTable(const std::string& tblname, Schema& sch, Transaction* tx) {
		tblmgr->createTable(tblname, &sch, tx);
	}

	Layout* getLayout(const std::string& tblname, Transaction* tx) {
		return tblmgr->getLayout(tblname, tx);
	}

	void createView(const std::string& viewname, const std::string& viewdef, Transaction* tx) {
		viewmgr->createView(viewname, viewdef, tx);
	}

	std::string getViewDef(const std::string& viewname, Transaction* tx) {
		return viewmgr->getViewDef(viewname, tx);
	}

	void createIndex(const std::string& idxname, const std::string& tblname,
		const std::string& fldname, Transaction* tx) {
		idxmgr->createIndex(idxname, tblname, fldname, tx);
	}

	std::unordered_map<std::string, IndexInfo*> getIndexInfo(const std::string& tblname, Transaction* tx) {
		return idxmgr->getIndexInfo(tblname, tx);
	}

	StatInfo getStatInfo(const std::string& tblname, Layout* layout, Transaction* tx) {
		return statmgr->getStatInfo(tblname, layout, tx);
	}
private:
	static TableMgr*  tblmgr;
	static ViewMgr*   viewmgr;
	static StatMgr*   statmgr;
	static IndexMgr*  idxmgr;
};

#endif // !__METADATA_MGR_H_
