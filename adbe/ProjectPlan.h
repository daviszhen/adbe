#ifndef __PROJECT_PLAN_H__
#define __PROJECT_PLAN_H__

#include "Plan.h"

class ProjectPlan : public Plan{
public:
	ProjectPlan(Plan* p, const std::list<std::string>& fieldlist) {
		this->p = p;
		for (const std::string& fldname : fieldlist)
			sch.add(fldname, p->schema());
	}

	Scan* open() {
		Scan* s = p->open();
		return new ProjectScan(s, sch.fields());
	}

	int blocksAccessed() {
		return p->blocksAccessed();
	}

	int recordsOutput() {
		return p->recordsOutput();
	}

	int distinctValues(const std::string& fldname) {
		return p->distinctValues(fldname);
	}

	Schema schema() {
		return sch;
	}
private:
	Plan* p;
	Schema sch;
};

#endif // !__PROJECT_PLAN_H__
