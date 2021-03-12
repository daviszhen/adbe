#ifndef __PLAN_H__
#define __PLAN_H__

#include "Scan.h"

class Plan {
public:
	virtual Scan*   open() = 0;

	virtual int    blocksAccessed() = 0;
	
	virtual int    recordsOutput() = 0;

	virtual int    distinctValues(const std::string& fldname) = 0;

	virtual Schema schema() = 0;
};
#endif // !__PLAN_H__
