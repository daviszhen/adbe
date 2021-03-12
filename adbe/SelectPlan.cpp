#include "SelectPlan.h"

int SelectPlan::distinctValues(const std::string& fldname) {
    Constant c;
    if (pred->equatesWithConstant(fldname,c))
        return 1;
    else {
        std::string fldname2;
        if (pred->equatesWithField(fldname, fldname2))
            return std::min(p->distinctValues(fldname), p->distinctValues(fldname2));
        else
            return p->distinctValues(fldname);
    }
}