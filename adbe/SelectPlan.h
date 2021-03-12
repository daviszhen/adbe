#ifndef __SELECT_PLAN_H__
#define __SELECT_PLAN_H__

#include "SelectScan.h"
#include "Plan.h"
#include "Predicate.h"

class SelectPlan : public Plan {
public:
	SelectPlan(Plan* p, Predicate* pred) {
		this->p = p;
		this->pred = pred;
	}

	Scan* open() {
		Scan* s = p->open();
		return new SelectScan(s, pred);
	}

	int blocksAccessed() {
		return p->blocksAccessed();
	}

	int recordsOutput() {
		return p->recordsOutput() / pred->reductionFactor(*p);
	}

	int distinctValues(const std::string& fldname);

	Schema schema() {
		return p->schema();
	}
private:
	Plan* p;
	Predicate* pred;
};

#endif // !__SELECT_PLAN_H__
