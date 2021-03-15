#ifndef __QUERY_DATA_H__
#define __QUERY_DATA_H__

#include "Predicate.h"
#include <string>
#include <list>
#include "ParserData.h"

class QueryData : public ParserData{
public:
	QueryData(const std::list<std::string>& fields, 
		const std::list<std::string>& tables, 
		Predicate* pred) {
		this->filds = fields;
		this->tbls = tables;
		this->prd = pred;
	}

	std::list<std::string> fields() {
		return filds;
	}

	std::list<std::string> tables() {
		return tbls;
	}

	Predicate* pred() {
		return prd;
	}

	std::string toString() {
		std::string result = "QueryData: select ";
		for (std::string fldname : filds)
			result += fldname + ", ";
		result = result.substr(0, result.length() - 2); //remove final comma
		result += " from ";
		for (std::string tblname : tbls)
			result += tblname + ", ";
		result = result.substr(0, result.length() - 2); //remove final comma
		std::string predstring = prd->toString();
		if (predstring != "")
			result += " where " + predstring;
		return result;
	}
private:
	std::list<std::string> filds;
	std::list<std::string> tbls;
	Predicate* prd;
};

#endif