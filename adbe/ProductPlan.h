#ifndef __PRODUCT_PLAN_H__
#define __PRODUCT_PLAN_H__

#include "Scan.h"
#include "Plan.h"

class ProductPlan : public Plan {
public:
	ProductPlan(Plan* p1, Plan* p2) {
		this->p1 = p1;
		this->p2 = p2;
		sch.addAll(p1->schema());
		sch.addAll(p2->schema());
	}

	Scan* open() {
		Scan* s1 = p1->open();
		Scan* s2 = p2->open();
		return new ProductScan(s1, s2);
	}

	int blocksAccessed() {
		return p1->blocksAccessed() + (p1->recordsOutput() * p2->blocksAccessed());
	}

	int recordsOutput() {
		return p1->recordsOutput() * p2->recordsOutput();
	}

	int distinctValues(const std::string& fldname);

	Schema schema() {
		return sch;
	}
private:
	Plan* p1, *p2;
	Schema sch;
};

class OptimizedProductPlan : public Plan {
public:
	OptimizedProductPlan(Plan* p1, Plan* p2) {
		Plan* prod1 = new ProductPlan(p1, p2);
		Plan* prod2 = new ProductPlan(p2, p1);
		int b1 = prod1->blocksAccessed();
		int b2 = prod2->blocksAccessed();
		bestplan = (b1 < b2) ? prod1 : prod2;
	}

	Scan* open() {
		return bestplan->open();
	}

	int blocksAccessed() {
		return bestplan->blocksAccessed();
	}

	int recordsOutput() {
		return bestplan->recordsOutput();
	}

	int distinctValues(const std::string& fldname) {
		return bestplan->distinctValues(fldname);
	}

	Schema schema() {
		return bestplan->schema();
	}
private:
	Plan* bestplan;
};

#endif // !__PRODUCT_PLAN_H__
