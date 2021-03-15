#ifndef __CREATE_VIEW_DATA_H__
#define __CREATE_VIEW_DATA_H__

#include "QueryData.h"

class CreateViewData : public ParserData{
public:
	CreateViewData(const std::string& viewname, QueryData* qrydata) {
		this->viewname = viewname;
		this->qrydata = qrydata;
	}

	std::string viewName() {
		return viewname;
	}

	std::string viewDef() {
		return qrydata->toString();
	}

	std::string toString() {
		std::string ans("CreateViewData:");
		ans += viewname + ",";
		ans += qrydata->toString();
		return ans;
	}
private:
	std::string viewname;
	QueryData* qrydata;
};
#endif // !__CREATE_VIEW_DATA_H__
