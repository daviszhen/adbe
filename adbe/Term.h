#ifndef __TERM_H__
#define __TERM_H__

#include "Scan.h"
#include "Plan.h"

//表达式运算
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

	//如果2个表达式在Scan上计算出相同的Constant返回true
	bool isSatisfied(Scan& s) {
		Constant lhsval = lhs->evaluate(s);
		Constant rhsval = rhs->evaluate(s);
		return rhsval == lhsval;
	}

	//需要查询计划的知识
	int reductionFactor(Plan& p);

	//检测term是否是"字段名=常数"的形式
	//是，返回常数
	bool equatesWithConstant(const std::string& fldname, Constant& out);

	//检测term是否是"字段名1=字段名2"的形式
	//是，返回字段名2
	bool equatesWithField(const std::string& fldname, std::string& out);

	//两个表达式都适用于Schema就返回true
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
