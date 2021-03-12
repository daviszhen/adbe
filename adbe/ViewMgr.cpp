#include "ViewMgr.h"
#include "Scan.h"
ViewMgr::ViewMgr(bool isNew, TableMgr* tblMgr, Transaction* tx) {
    this->tblMgr = tblMgr;
    if (isNew) {
        Schema sch;
        sch.addStringField("viewname", TableMgr::MAX_NAME);
        sch.addStringField("viewdef", MAX_VIEWDEF);
        tblMgr->createTable("viewcat", &sch, tx);
    }
}

void ViewMgr::createView(const std::string& vname, const std::string& vdef, Transaction* tx) {
    Layout* layout = tblMgr->getLayout("viewcat", tx);
    TableScan* ts = new TableScan(tx, "viewcat", layout);
    ts->insert();
    ts->setString("viewname", vname);
    ts->setString("viewdef", vdef);
    ts->close();

    delete ts;
}

std::string ViewMgr::getViewDef(const std::string& vname, Transaction* tx) {
    std::string result;
    Layout* layout = tblMgr->getLayout("viewcat", tx);
    TableScan* ts = new TableScan(tx, "viewcat", layout);
    while (ts->next())
        if (ts->getString("viewname") == vname) {
            result = ts->getString("viewdef");
            break;
        }
    ts->close();

    delete ts;
    return result;
}