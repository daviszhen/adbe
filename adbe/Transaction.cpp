#include "Transaction.h"

int Transaction::nextTxNum = 0;
mysql_mutex_t Transaction::transNextTxMutex;

void Transaction::init() {
    mysql_mutex_init(NULL, &transNextTxMutex, MY_MUTEX_INIT_FAST);
}

void Transaction::deinit() {
    mysql_mutex_destroy(&transNextTxMutex);
}

int Transaction::nextTxNumber() {
    mysql_mutex_lock(&transNextTxMutex);
    int n = ++nextTxNum;
    mysql_mutex_unlock(&transNextTxMutex);
    return n;
}

Transaction::Transaction(FileMgr* fm, LogMgr* lm, BufferMgr* bm, LockTable* locktbl) {
    this->fm = fm;
    this->bm = bm;
    txnum = nextTxNumber();
    recoveryMgr = new RecoveryMgr(this, txnum, lm, bm);
    concurMgr = new ConcurrencyMgr(locktbl);
    mybuffers = new BufferList(bm);
}

Transaction::~Transaction(){
    delete recoveryMgr;
    delete concurMgr;
    delete mybuffers;
}

void Transaction::commit() {
    recoveryMgr->commit();
    sql_print_information("transaction %d committed \n", txnum);
    concurMgr->release();
    mybuffers->unpinAll();
}

void Transaction::rollback() {
    recoveryMgr->rollback();
    sql_print_information("transaction %d rolled back \n", txnum);
    concurMgr->release();
    mybuffers->unpinAll();
}

void Transaction::recover() {
    bm->flushAll(txnum);
    recoveryMgr->recover();
}

bool Transaction::pin(BlockId* blk) {
    return mybuffers->pin(blk);
}

void Transaction::unpin(BlockId* blk) {
    mybuffers->unpin(blk);
}

TransStatus Transaction::getInt(BlockId* blk, int offset, int& out) {
    bool locksucc = concurMgr->sLock(blk);
    if (!locksucc) {
        return TX_SLOCK_FAILED;
    }
    Buffer* buff = mybuffers->getBuffer(blk);
    out =  buff->contents()->getInt(offset);
    return TX_SUCC;
}

TransStatus Transaction::getString(BlockId* blk, int offset, std::string& out) {
    bool locksucc = concurMgr->sLock(blk);
    if (!locksucc) {
        return TX_SLOCK_FAILED;
    }
    Buffer* buff = mybuffers->getBuffer(blk);
    out =  buff->contents()->getString(offset);
    return TX_SUCC;
}

TransStatus Transaction::setInt(BlockId* blk, int offset, int val, bool okToLog) {
    bool locksucc = concurMgr->xLock(blk);
    if (!locksucc) {
        return TX_XLOCK_FAILED;
    }
    Buffer* buff = mybuffers->getBuffer(blk);
    int lsn = -1;
    if (okToLog)
        lsn = recoveryMgr->setInt(buff, offset, val);
    Page* p = buff->contents();
    p->setInt(offset, val);
    buff->setModified(txnum, lsn);
    return TX_SUCC;
}

TransStatus Transaction::setString(BlockId* blk, int offset, const std::string& val, bool okToLog) {
    bool locksucc = concurMgr->xLock(blk);
    if (!locksucc) {
        return TX_XLOCK_FAILED;
    }
    Buffer* buff = mybuffers->getBuffer(blk);
    int lsn = -1;
    if (okToLog)
        lsn = recoveryMgr->setString(buff, offset, val);
    Page* p = buff->contents();
    p->setString(offset, val);
    buff->setModified(txnum, lsn);
    return TX_SUCC;
}

TransStatus Transaction::size(const std::string& filename, int& out) {
    BlockId* dummyblk = BlockId::New(filename, END_OF_FILE);
    bool locksucc = concurMgr->sLock(dummyblk);
    if (!locksucc) {
        return TX_SLOCK_FAILED;
    }
    out = fm->length(filename);
    return TX_SUCC;
}

TransStatus Transaction::append(const std::string& filename, BlockId*& out) {
    BlockId* dummyblk = BlockId::New(filename, END_OF_FILE);
    bool locksucc = concurMgr->xLock(dummyblk);
    if (!locksucc) {
        return TX_SLOCK_FAILED;
    }
    out = fm->append(filename);
    return TX_SUCC;
}