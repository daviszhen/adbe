#ifndef __CREATE_INDEX_DATA_H__
#define __CREATE_INDEX_DATA_H__

#include <string>
#include "ParserData.h"

class CreateIndexData : public ParserData{
public:
	CreateIndexData(const std::string& idxname, const std::string& tblname, const std::string& fldname) {
		this->idxname = idxname;
		this->tblname = tblname;
		this->fldname = fldname;
	}

	std::string indexName() {
		return idxname;
	}

	std::string tableName() {
		return tblname;
	}

	std::string fieldName() {
		return fldname;
	}

	std::string toString() {
		return "CreateIndexData:" + idxname + "," + tblname + "," + fldname;
	}
private:
	std::string idxname, tblname, fldname;
};

#endif // !__CREATE_INDEX_DATA_H__
