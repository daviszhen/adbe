#ifndef __CREATE_TABLE_DATA_H__
#define __CREATE_TABLE_DATA_H__

#include "Schema.h"

class CreateTableData: public ParserData {
public:
	CreateTableData(const std::string& tblname, Schema& sch) {
		this->tblname = tblname;
		this->sch = sch;
	}

	std::string tableName() {
		return tblname;
	}

	Schema newSchema() {
		return sch;
	}

	std::string toString() {
		std::string ans("CreateTableData:");
		ans += tblname + ",";
		ans += sch.toString();
		return ans;
	}
private:
	std::string tblname;
	Schema sch;
};
#endif // !__CREATE_TABLE_DATA_H__
