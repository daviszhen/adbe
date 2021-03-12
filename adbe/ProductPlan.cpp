#include "ProductPlan.h"

int ProductPlan::distinctValues(const std::string& fldname) {
    if (p1->schema().hasField(fldname))
        return p1->distinctValues(fldname);
    else
        return p2->distinctValues(fldname);
}