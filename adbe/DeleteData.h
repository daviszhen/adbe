#ifndef __DELETE_DATA_H__
#define __DELETE_DATA_H__

#include "Predicate.h"

class DeleteData:public ParserData {
public:

    DeleteData(const std::string& tblname, Predicate* pred) {
        this->tblname = tblname;
        this->prd = pred;
    }

    std::string tableName() {
        return tblname;
    }

    Predicate* pred() {
        return prd;
    }

    std::string toString() {
        std::string ans("DeleteData:");
        ans += tblname + ",";
        ans += prd->toString();
        return ans; 
    }
private:
    std::string tblname;
    Predicate* prd;
};

#endif // !__DELETE_DATA_H__