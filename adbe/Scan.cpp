#include "Scan.h"

Constant Expression::evaluate(Scan& s) {
    return (val != nullptr) ? *val : s.getVal(fldname);
}

TableScan::TableScan(Transaction* tx, const std::string& tblname, Layout* layout) {
    this->tx = tx;
    this->layout = layout;
    this->rp = nullptr;
    filename = tblname + ".tbl";
    int ival;
    assert(TX_SUCC == tx->size(filename,ival));
    if (ival == 0)
        moveToNewBlock();
    else
        moveToBlock(0);
}

TableScan::~TableScan() {
    delete rp;
}

void TableScan::beforeFirst() {
    moveToBlock(0);
}

bool TableScan::next() {
    currentslot = rp->nextAfter(currentslot);
    while (currentslot < 0) {
        if (atLastBlock())
            return false;
        moveToBlock(rp->block()->number() + 1);
        currentslot = rp->nextAfter(currentslot);
    }
    return true;
}

int TableScan::getInt(const std::string& fldname) {
    int ival;
    assert(rp->getInt(currentslot, fldname,ival));
    return ival;
}

std::string TableScan::getString(const std::string& fldname) {
    std::string sval;
    assert(rp->getString(currentslot, fldname,sval));
    return sval;
}

Constant TableScan::getVal(const std::string& fldname) {
    if (layout->schema().type(fldname) == INTEGER)
        return Constant(getInt(fldname));
    else
        return Constant(getString(fldname));
}

bool TableScan::hasField(const std::string& fldname) {
    return layout->schema().hasField(fldname);
}

void TableScan::close() {
    if (rp != nullptr)
        tx->unpin(rp->block());
}

void TableScan::setInt(const std::string& fldname, int val) {
    assert(rp->setInt(currentslot, fldname, val));
}

void TableScan::setString(const std::string& fldname, const std::string& val) {
    assert(rp->setString(currentslot, fldname, val));
}

void TableScan::setVal(const std::string& fldname, Constant& val) {
    if (layout->schema().type(fldname) == INTEGER)
        setInt(fldname, val.asInt());
    else
        setString(fldname, val.asString());
}

void TableScan::insert() {
    currentslot = rp->insertAfter(currentslot);
    while (currentslot < 0) {
        if (atLastBlock())
            moveToNewBlock();
        else
            moveToBlock(rp->block()->number() + 1);
        currentslot = rp->insertAfter(currentslot);
    }
}

void TableScan::remove() {
    rp->remove(currentslot);
}

void TableScan::moveToRid(RID& rid) {
    close();
    BlockId* blk = BlockId::New(filename, rid.blockNumber());
    switchRecordPage(new RecordPage(tx, blk, layout));
    currentslot = rid.slot();
}

RID TableScan::getRid() {
    return RID(rp->block()->number(), currentslot);
}

void TableScan::moveToBlock(int blknum) {
    close();
    BlockId* blk = BlockId::New(filename, blknum);
    switchRecordPage(new RecordPage(tx, blk, layout));
    currentslot = -1;
}

void TableScan::moveToNewBlock() {
    close();
    BlockId* blk = nullptr;
    assert(TX_SUCC == tx->append(filename,blk));
    switchRecordPage(new RecordPage(tx, blk, layout));
    assert(rp->format());
    currentslot = -1;
}

void TableScan::switchRecordPage(RecordPage* rp) {
    if (this->rp) {
        delete this->rp;
    }
    this->rp = rp;
}

bool TableScan::atLastBlock() {
    int block_size;
    assert(TX_SUCC == tx->size(filename,block_size));
    return rp->block()->number() == block_size - 1;
}