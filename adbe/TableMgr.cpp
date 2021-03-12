#include "TableMgr.h"
#include "Scan.h"
#include <unordered_map>

TableMgr::TableMgr(bool isNew, Transaction* tx) {
    Schema tcatSchema;
    tcatSchema.addStringField("tblname", MAX_NAME);
    tcatSchema.addIntField("slotsize");
    tcatLayout = new Layout(tcatSchema);

    Schema fcatSchema;
    fcatSchema.addStringField("tblname", MAX_NAME);
    fcatSchema.addStringField("fldname", MAX_NAME);
    fcatSchema.addIntField("type");
    fcatSchema.addIntField("length");
    fcatSchema.addIntField("offset");
    fcatLayout = new Layout(fcatSchema);

    if (isNew) {
        createTable("tblcat", &tcatSchema, tx);
        createTable("fldcat", &fcatSchema, tx);
    }
}

void TableMgr::createTable(const std::string& tblname, Schema* sch, Transaction* tx){
    Layout* layout = new Layout(*sch);
    // insert one record into tblcat
    TableScan* tcat = new TableScan(tx, "tblcat", tcatLayout);
    tcat->insert();
    tcat->setString("tblname", tblname);
    tcat->setInt("slotsize", layout->slotSize());
    tcat->close();

    // insert a record into fldcat for each field
    TableScan* fcat = new TableScan(tx, "fldcat", fcatLayout);
    for (const std::string& fldname : sch->fields()) {
        fcat->insert();
        fcat->setString("tblname", tblname);
        fcat->setString("fldname", fldname);
        fcat->setInt("type", sch->type(fldname));
        fcat->setInt("length", sch->length(fldname));
        fcat->setInt("offset", layout->offset(fldname));
    }
    fcat->close();

    delete tcat;
    delete fcat;
}

Layout* TableMgr::getLayout(const std::string& tblname, Transaction* tx) {
    int size = -1;
    TableScan* tcat = new TableScan(tx, "tblcat", tcatLayout);
    while (tcat->next())
        if (tcat->getString("tblname") == tblname) {
            size = tcat->getInt("slotsize");
            break;
        }
    tcat->close();

    Schema sch;
    std::unordered_map<std::string, int> offsets;
    TableScan* fcat = new TableScan(tx, "fldcat", fcatLayout);
    while (fcat->next())
        if (fcat->getString("tblname") == tblname) {
            std::string fldname = fcat->getString("fldname");
            int fldtype = fcat->getInt("type");
            int fldlen = fcat->getInt("length");
            int offset = fcat->getInt("offset");
            offsets[fldname]=offset;
            //sql_print_information("tblename %s fidname %s type %d length %d offset %d\n",
            //    tblname.c_str(),fldname.c_str(),fldtype,fldlen,offset);
            sch.addField(fldname, static_cast<FieldType>(fldtype), fldlen);
        }
    fcat->close();

    delete tcat;
    delete fcat;
    return new Layout(sch, offsets, size);
}