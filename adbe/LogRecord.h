#ifndef __LOG_RECORD_H__
#define __LOG_RECORD_H__

#include <string>

#include "bytebuffer.h"
#include "LogMgr.h"
#include "blockid.h"
#include "Transaction.h"

class Transaction;
class CheckpointRecord;
class CommitRecord;
class RollbackRecord;
class SetIntRecord;
class SetStringRecord;
class StartRecord;

enum LogRecordType
{
    INVALID_TYPE = -1,
    CHECKPOINT = 0, 
    START = 1,
    COMMIT = 2, 
    ROLLBACK = 3,
    SETINT = 4, 
    SETSTRING = 5
};

class LogRecord {
public:

    //日志记录的类型
    virtual LogRecordType op() { return INVALID_TYPE; }

    //日志记录中的事务id
    virtual int txNumber() { return -1;  };

    //undo日志记录中的操作
    virtual void undo(Transaction* tx) {}

    virtual std::string to_str() { return "null string"; }

    //分析缓冲区中的日志记录类型
    static LogRecord* createLogRecord(ByteBuffer* bytes);
};

class CheckpointRecord : public LogRecord {
public:
    CheckpointRecord() {}

    LogRecordType op() {
        return CHECKPOINT;
    }

    std::string to_str() {
        return "<CHECKPOINT>";
    }

    //向LogMgr中写入一个checkpoint记录
    static int writeToLog(LogMgr* lm) {
        ByteBuffer* rec = ByteBuffer::New(sizeof(int));
        Page* p = Page::New(rec);
        p->setInt(0, CHECKPOINT);
        int lsn = lm->append(rec);

        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
};

class CommitRecord : public LogRecord {
public:
    CommitRecord(Page* p) {
        int tpos = sizeof(int);
        txnum = p->getInt(tpos);
    }

    LogRecordType op(){
        return COMMIT;
    }

    int txNumber() {
        return txnum;
    }

    std::string to_str() {
        return "<COMMIT " + std::to_string(txnum) + ">";
    }

    static int writeToLog(LogMgr* lm, int txnum) {
        ByteBuffer* rec = ByteBuffer::New(2 * sizeof(int));
        Page* p = Page::New(rec);
        p->setInt(0, COMMIT);
        p->setInt(sizeof(int), txnum);
        int lsn =  lm->append(rec);
        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
private:
    int txnum;
};

class RollbackRecord : public LogRecord {
public:
    RollbackRecord(Page* p) {
        int tpos = sizeof(int);
        txnum = p->getInt(tpos);
    }

    LogRecordType op() {
        return ROLLBACK;
    }

    int txNumber() {
        return txnum;
    }

    std::string to_str() {
        return "<ROLLBACK " + std::to_string(txnum) + ">";
    }

    static int writeToLog(LogMgr* lm, int txnum) {
        ByteBuffer* rec = ByteBuffer::New(2 * sizeof(int));
        Page* p = Page::New(rec);
        p->setInt(0, ROLLBACK);
        p->setInt(sizeof(int), txnum);
        int lsn = lm->append(rec);
        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
private:
    int txnum;
};

class SetIntRecord : public LogRecord {
public:
    SetIntRecord(Page* p) {
        int tpos = sizeof(int);
        txnum = p->getInt(tpos);
        int fpos = tpos + sizeof(int);
        std::string filename = p->getString(static_cast<int>(fpos));
        int bpos = fpos + Page::maxLength(static_cast<int>(filename.size()));
        int blknum = p->getInt(bpos);
        blk = BlockId::New(filename, blknum);
        int opos = bpos + sizeof(int);
        offset = p->getInt(opos);
        int vpos = opos + sizeof(int);
        val = p->getInt(vpos);
    }

    ~SetIntRecord() {
        delete blk;
    }

    LogRecordType op() {
        return SETINT;
    }

    int txNumber() {
        return txnum;
    }

    std::string to_str() {
        return "<SETINT " + std::to_string(txnum) + " " + blk->to_str() + " " + std::to_string(offset) + " " + std::to_string(val) + ">";
    }

    //撤销日志记录中的操作
    void undo(Transaction* tx);

    static int writeToLog(LogMgr* lm, int txnum, BlockId* blk, int offset, int val) {
        int tpos = sizeof(int);
        int fpos = tpos + sizeof(int);
        int bpos = fpos + Page::maxLength(blk->fileName().size());
        int opos = bpos + sizeof(int);
        int vpos = opos + sizeof(int);
        ByteBuffer* rec = ByteBuffer::New(vpos + sizeof(int));
        Page* p = Page::New(rec);
        p->setInt(0, SETINT);
        p->setInt(tpos, txnum);
        p->setString(fpos, blk->fileName());
        p->setInt(bpos, blk->number());
        p->setInt(opos, offset);
        p->setInt(vpos, val);
        int lsn = lm->append(rec);
        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
private:
    int txnum, offset, val;
    BlockId* blk;
};

class SetStringRecord : public LogRecord {
public:
    SetStringRecord(Page* p) {
        int tpos = sizeof(int);
        txnum = p->getInt(tpos);
        int fpos = tpos + sizeof(int);
        std::string filename = p->getString(fpos);
        int bpos = fpos + Page::maxLength(filename.size());
        int blknum = p->getInt(bpos);
        blk = BlockId::New(filename, blknum);
        int opos = bpos + sizeof(int);
        offset = p->getInt(opos);
        int vpos = opos + sizeof(int);
        val = p->getString(vpos);
    }

    ~SetStringRecord() {
        delete blk;
    }

    LogRecordType op() {
        return SETSTRING;
    }

    int txNumber() {
        return txnum;
    }

    std::string to_str() {
        return "<SETSTRING " + std::to_string(txnum) + " " + blk->to_str() + " " + std::to_string(offset) + " " + val + ">";
    }

    void undo(Transaction* tx);

    static int writeToLog(LogMgr* lm, int txnum, BlockId* blk, int offset, const std::string& val) {
        int tpos = sizeof(int);
        int fpos = tpos + sizeof(int);
        int bpos = fpos + Page::maxLength(blk->fileName().size());
        int opos = bpos + sizeof(int);
        int vpos = opos + sizeof(int);
        int reclen = vpos + Page::maxLength(val.size());
        ByteBuffer* rec = ByteBuffer::New(reclen);
        Page* p = Page::New(rec);
        p->setInt(0, SETSTRING);
        p->setInt(tpos, txnum);
        p->setString(fpos, blk->fileName());
        p->setInt(bpos, blk->number());
        p->setInt(opos, offset);
        p->setString(vpos, val);
        int lsn = lm->append(rec);
        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
private:
    int txnum, offset;
    std::string val;
    BlockId* blk;
};

class StartRecord : public LogRecord {
public:
    StartRecord(Page* p) {
        int tpos = sizeof(int);
        txnum = p->getInt(tpos);
    }

    LogRecordType op() {
        return START;
    }

    int txNumber() {
        return txnum;
    }

    std::string to_str() {
        return "<START " + std::to_string(txnum) + ">";
    }

    static int writeToLog(LogMgr* lm, int txnum) {
        ByteBuffer* rec = ByteBuffer::New(2 * sizeof(int));
        Page* p = Page::New(rec);
        p->setInt(0, START);
        p->setInt(sizeof(int), txnum);
        int lsn = lm->append(rec);
        p->extractBuffer();
        delete p;
        delete rec;
        return lsn;
    }
private:
    int txnum;
};

#endif // !__LOG_RECORD_H__
