#include "RecordPage.h"

RecordPage::RecordPage(Transaction* tx, BlockId* blk, Layout* layout) {
    this->tx = tx;
    this->blk = blk;
    this->layout = layout;
    if (!tx->pin(blk)) {
        sql_print_error("RecordPage() pin blk failed.");
        exit(-1);
    }
}

bool RecordPage::getInt(int slot, const std::string& fldname,int& out) {
    int fldpos = offset(slot) + layout->offset(fldname);
    return tx->getInt(blk, fldpos,out) == TX_SUCC;
}

bool RecordPage::getString(int slot, const std::string& fldname,std::string &out) {
    int fldpos = offset(slot) + layout->offset(fldname);
    return tx->getString(blk, fldpos, out) ==  TX_SUCC;
}

bool RecordPage::setInt(int slot, const std::string& fldname, int val) {
    int fldpos = offset(slot) + layout->offset(fldname);
    return tx->setInt(blk, fldpos, val, true) == TX_SUCC;
}

bool RecordPage::setString(int slot, const std::string& fldname, const std::string& val) {
    int fldpos = offset(slot) + layout->offset(fldname);
    return tx->setString(blk, fldpos, val, true) == TX_SUCC;
}

bool RecordPage::remove(int slot) {
    return setFlag(slot, EMPTY);
}

bool RecordPage::format() {
    int slot = 0;
    while (isValidSlot(slot)) {
        if (TX_SUCC != tx->setInt(blk, offset(slot), EMPTY, false)) {
            return false;
        }
        Schema& sch = layout->schema();
        for (const std::string fldname : sch.fields()) {
            int fldpos = offset(slot) + layout->offset(fldname);
            if (sch.type(fldname) == INTEGER) {
                if (TX_SUCC != tx->setInt(blk, fldpos, 0, false)) {
                    return false;
                }
            }
            else {
                if (TX_SUCC != tx->setString(blk, fldpos, "", false)) {
                    return false;
                }
            }
        }
        slot++;
    }
    return true;
}

int RecordPage::nextAfter(int slot) {
    return searchAfter(slot, USED);
}

int RecordPage::insertAfter(int slot) {
    int newslot = searchAfter(slot, EMPTY);
    if (newslot >= 0) {
        bool ret = setFlag(newslot, USED);
        if (ret) {
            return newslot;
        }
        else {
            return -2;
        }
    }
    return -1;
}

BlockId* RecordPage::block() {
    return blk;
}

bool RecordPage::setFlag(int slot, int flag) {
    if (tx->setInt(blk, offset(slot), flag, true) == TX_SUCC) {
        return true;
    }
    return false;
}


int RecordPage::searchAfter(int slot, int flag) {
    slot++;
    while (isValidSlot(slot)) {
        int ival;
        if (tx->getInt(blk, offset(slot), ival) == TX_SUCC) {
            if (ival == flag)
                return slot;
        }
        else {
            return -2;
        }
        slot++;
    }
    return -1;
}

bool RecordPage::isValidSlot(int slot) {
    return offset(slot + 1) <= tx->blockSize();
}

int RecordPage::offset(int slot) {
    return slot * layout->slotSize();
}