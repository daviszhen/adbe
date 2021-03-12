#include "Term.h"

int Term::reductionFactor(Plan& p) {
    std::string lhsName, rhsName;
    if (lhs->isFieldName() && rhs->isFieldName()) {
        lhsName = lhs->asFieldName();
        rhsName = rhs->asFieldName();
        return std::max(p.distinctValues(lhsName), p.distinctValues(rhsName));
    }
    if (lhs->isFieldName()) {
        lhsName = lhs->asFieldName();
        return p.distinctValues(lhsName);
    }
    if (rhs->isFieldName()) {
        rhsName = rhs->asFieldName();
        return p.distinctValues(rhsName);
    }
    // otherwise, the term equates constants
    if (lhs->asConstant() == rhs->asConstant())
        return 1;
    else
        return INT_MAX;
}

 bool Term::equatesWithConstant(const std::string& fldname, Constant& out) {
     if (lhs->isFieldName() &&
         lhs->asFieldName() == fldname &&
         !rhs->isFieldName()) {
         out = rhs->asConstant();
         return true;
     }
    else if (rhs->isFieldName() &&
        rhs->asFieldName() == fldname &&
        !lhs->isFieldName()) {
        out = lhs->asConstant();
        return true;
    }
    else
        return false;
}

bool Term::equatesWithField(const std::string& fldname, std::string& out) {
    if (lhs->isFieldName() &&
        lhs->asFieldName() == fldname &&
        rhs->isFieldName()) {
        out = rhs->asFieldName();
        return true;
    }
    else if (rhs->isFieldName() &&
        rhs->asFieldName() == fldname &&
        lhs->isFieldName()) {
        out = lhs->asFieldName();
        return true;
    }
    else
        return false;
}