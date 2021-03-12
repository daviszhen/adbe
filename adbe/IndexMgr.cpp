#include "IndexMgr.h"
#include "Scan.h"


IndexInfo::IndexInfo(const std::string& idxname, const std::string& fldname,
    Schema& tblSchema,
    Transaction* tx, StatInfo& si) {
    this->idxname = idxname;
    this->fldname = fldname;
    this->tx = tx;
    this->tblSchema = tblSchema;
    this->idxLayout = createIdxLayout();
    this->si = si;
}

Index* IndexInfo::open() {
    return new HashIndex(tx, idxname, idxLayout);
    //    return new BTreeIndex(tx, idxname, idxLayout);
}

int IndexInfo::blocksAccessed() {
    int rpb = tx->blockSize() / idxLayout->slotSize();
    int numblocks = si.recordsOutput() / rpb;
    return HashIndex::searchCost(numblocks, rpb);
    //    return BTreeIndex.searchCost(numblocks, rpb);
}

int IndexInfo::recordsOutput() {
    return si.recordsOutput() / si.distinctValues(fldname);
}

int IndexInfo::distinctValues(const std::string& fname) {
    return fldname == fname ? 1 : si.distinctValues(fldname);
}

Layout* IndexInfo::createIdxLayout() {
    Schema sch;
    sch.addIntField("block");
    sch.addIntField("id");
    if (tblSchema.type(fldname) == INTEGER)
        sch.addIntField("dataval");
    else {
        int fldlen = tblSchema.length(fldname);
        sch.addStringField("dataval", fldlen);
    }
    return new Layout(sch);
}

IndexMgr::IndexMgr(bool isnew, TableMgr* tblmgr, StatMgr* statmgr, Transaction* tx) {
    if (isnew) {
        Schema sch;
        sch.addStringField("indexname", TableMgr::MAX_NAME);
        sch.addStringField("tablename", TableMgr::MAX_NAME);
        sch.addStringField("fieldname", TableMgr::MAX_NAME);
        tblmgr->createTable("idxcat", &sch, tx);
    }
    this->tblmgr = tblmgr;
    this->statmgr = statmgr;
    layout = tblmgr->getLayout("idxcat", tx);
}

void IndexMgr::createIndex(const std::string& idxname,
    const std::string& tblname,
    const std::string& fldname, Transaction* tx){
    TableScan ts(tx, "idxcat", layout);
    ts.insert();
    ts.setString("indexname", idxname);
    ts.setString("tablename", tblname);
    ts.setString("fieldname", fldname);
    ts.close();
}

std::unordered_map<std::string, IndexInfo*> IndexMgr::getIndexInfo(const std::string& tblname, Transaction* tx) {
    std::unordered_map<std::string, IndexInfo*> result;
    TableScan ts(tx, "idxcat", layout);
    while (ts.next())
        if (ts.getString("tablename") == tblname) {
            std::string idxname = ts.getString("indexname");
            std::string fldname = ts.getString("fieldname");
            Layout* tblLayout = tblmgr->getLayout(tblname, tx);
            StatInfo tblsi = statmgr->getStatInfo(tblname, tblLayout, tx);
            result[fldname]= new IndexInfo(idxname, fldname, tblLayout->schema(), tx, tblsi);
        }
    ts.close();
    return result;
}