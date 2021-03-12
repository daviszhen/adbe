#include "Predicate.h"

Predicate * Predicate::selectSubPred(Schema& sch) {
    Predicate* result = new Predicate();
    for (Term* t : terms)
        if (t->appliesTo(sch))
            result->terms.push_back(t);
    if (result->terms.size() == 0) {
        delete result;
        return nullptr;
    }
    else
        return result;
}

Predicate* Predicate::joinSubPred(Schema& sch1, Schema& sch2) {
    Predicate* result = new Predicate();
    Schema newsch;
    newsch.addAll(sch1);
    newsch.addAll(sch2);
    for (Term* t : terms)
        if (!t->appliesTo(sch1) &&
            !t->appliesTo(sch2) &&
            t->appliesTo(newsch))
            result->terms.push_back(t);
    if (result->terms.size() == 0) {
        delete result;
        return nullptr;
    }
    else
        return result;
}

 bool Predicate::equatesWithConstant(const std::string& fldname, Constant& out) {
    for (Term* t : terms) {
        bool ret = t->equatesWithConstant(fldname,out);
        if (ret)
            return true;
    }
    return false;
}

 bool Predicate::equatesWithField(const std::string& fldname,std::string& out) {
     for (Term* t : terms) {
         bool ret = t->equatesWithField(fldname,out);
         if (ret)
             return true;
     }
     return false;
 }

 std::string Predicate::toString() {
     std::string ans;
     int i = 0;
     for (auto t : terms) {
         if (i++) {
             ans += " and ";
         }
         ans += t->toString();
     }
     return ans;
 }