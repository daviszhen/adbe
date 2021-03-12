#ifndef __PREDICATE_H__
#define __PREDICATE_H__

#include "Scan.h"
#include "Plan.h"
#include "Term.h"

#include <list>

//ν��
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

	//ν�ʽ�
	void conjoinWith(const Predicate& pred) {
		terms.insert(terms.end(),pred.terms.begin(),pred.terms.end());
	}

	//ÿһ��Term��Scan������Ϊtrueʱ������true
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

	//����������Schema����ν��
	Predicate* selectSubPred(Schema& sch);

	//���� ������(sch1 �� sch2) �� ��������sch1 �� ��������sch2
	Predicate* joinSubPred(Schema& sch1, Schema& sch2);

	//ȷ���Ƿ���term����"F=c",�У�����c
	bool equatesWithConstant(const std::string& fldname,Constant& out);

	//ȷ���Ƿ���term����"F1=F2",�У�����F2
	bool equatesWithField(const std::string& fldname, std::string& out);

	std::string toString();
private:
	std::list<Term*> terms;
};

#endif // !__PREDICATE
