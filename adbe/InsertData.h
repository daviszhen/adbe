#ifndef __INSERT_DATA_H__
#define __INSERT_DATA_H__

#include <string>
#include <list>

#include "Scan.h"

class InsertData : public ParserData {
public:
	InsertData(const std::string& tblname, 
			const std::list<std::string>& flds, 
			const std::list<Constant>& vals) {
		this->tblname = tblname;
		this->flds = flds;
		this->values = vals;
	}

	std::string tableName() {
		return tblname;
	}

	std::list<std::string> fields() {
		return flds;
	}

	std::list<Constant> vals() {
		return values;
	}

	std::string toString() {
		std::string ans("InsertData:");
		ans += tblname + ",";
		for (auto& x : flds) {
			ans += x + ",";
		}
		for (auto& x : values) {
			ans += x.toString() + ",";
		}
		return ans;
	}
private:
	std::string tblname;
	std::list<std::string> flds;
	std::list<Constant> values;
};

#endif // !__INSERT_DATA_H__
