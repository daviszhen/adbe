#ifndef __MODIFY_DATA_H__
#define __MODIFY_DATA_H__

#include <string>

#include "Scan.h"
#include "Predicate.h"
#include "ParserData.h"

class ModifyData : public ParserData{
public:
	ModifyData(const std::string& tblname, const std::string& fldname,
		Expression* newval, Predicate* pred) {
		this->tblname = tblname;
		this->fldname = fldname;
		this->newval = newval;
		this->prd = pred;
	}

	std::string tableName() {
		return tblname;
	}

	std::string targetField() {
		return fldname;
	}

	Expression* newValue() {
		return newval;
	}

	Predicate* pred() {
		return prd;
	}

	std::string toString() {
		return "ModifyData:" + tblname + ", " + fldname +","+ newval->toString() + "," + prd->toString();
	}
private:
	 std::string tblname;
	 std::string fldname;
	 Expression* newval;
	 Predicate* prd;
};
#endif // __MODIFY_DATA_H__
