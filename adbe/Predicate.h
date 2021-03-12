#ifndef __PREDICATE_H__
#define __PREDICATE_H__

#include "Scan.h"
#include "Plan.h"
#include "Term.h"

#include <list>

//谓词
class Predicate {
public:
	Predicate() {}

	Predicate(Term* t) {
		terms.push_back(t);
	}
	~Predicate() {
		for (auto p : terms) {
			delete p;
		}
	}

	//谓词交
	void conjoinWith(const Predicate& pred) {
		terms.insert(terms.end(),pred.terms.begin(),pred.terms.end());
	}

	//每一个Term在Scan都评估为true时，返回true
	bool isSatisfied(Scan& s) {
		for (Term* t : terms)
			if (!t->isSatisfied(s))
				return false;
		return true;
	}


	//tototootoo
	int reductionFactor(Plan& p) {
		int factor = 1;
		for (Term* t : terms)
			factor *= t->reductionFactor(p);
		return factor;
	}

	//构造适用于Schema的子谓词
	Predicate* selectSubPred(Schema& sch);

	//构造 适用于(sch1 并 sch2) 且 不适用于sch1 且 不适用于sch2
	Predicate* joinSubPred(Schema& sch1, Schema& sch2);

	//确定是否有term形如"F=c",有，返回c
	bool equatesWithConstant(const std::string& fldname,Constant& out);

	//确定是否有term形如"F1=F2",有，返回F2
	bool equatesWithField(const std::string& fldname, std::string& out);

	std::string toString();
private:
	std::list<Term*> terms;
};

#endif // !__PREDICATE
