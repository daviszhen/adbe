#ifndef __TERM_H__
#define __TERM_H__

#include "Scan.h"
#include "Plan.h"

//���ʽ����
class Term {
public:
	Term(Expression* lhs, Expression* rhs) {
		this->lhs = lhs;
		this->rhs = rhs;
	}

	~Term() {
		delete lhs;
		delete rhs;
	}

	//���2�����ʽ��Scan�ϼ������ͬ��Constant����true
	bool isSatisfied(Scan& s) {
		Constant lhsval = lhs->evaluate(s);
		Constant rhsval = rhs->evaluate(s);
		return rhsval == lhsval;
	}

	//��Ҫ��ѯ�ƻ���֪ʶ
	int reductionFactor(Plan& p);

	//���term�Ƿ���"�ֶ���=����"����ʽ
	//�ǣ����س���
	bool equatesWithConstant(const std::string& fldname, Constant& out);

	//���term�Ƿ���"�ֶ���1=�ֶ���2"����ʽ
	//�ǣ������ֶ���2
	bool equatesWithField(const std::string& fldname, std::string& out);

	//�������ʽ��������Schema�ͷ���true
	bool appliesTo(Schema& sch) {
		return lhs->appliesTo(sch) && rhs->appliesTo(sch);
	}

	std::string toString() {
		return lhs->toString() + "=" + rhs->toString();
	}
private:
	Expression* lhs, * rhs;
};

#endif // !__TERM_H__
