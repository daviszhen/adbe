#include "LogRecord.h"

// 撤销日志记录中的操作
void SetIntRecord::undo(Transaction * tx) {
    tx->pin(blk);
    tx->setInt(blk, offset, val, false); // don't log the undo!
    tx->unpin(blk);
}

void SetStringRecord::undo(Transaction* tx) {
    tx->pin(blk);
    tx->setString(blk, offset, val, false); // don't log the undo!
    tx->unpin(blk);
}

LogRecord* LogRecord::createLogRecord(ByteBuffer* bytes) {
    Page* p = Page::New(bytes);
    int type = p->getInt(0);
    switch (type)
    {
    case CHECKPOINT:
        return new CheckpointRecord();
    case START:
        return new StartRecord(p);
    case COMMIT:
        return new CommitRecord(p);
    case ROLLBACK:
        return new RollbackRecord(p);
    case SETINT:
        return new SetIntRecord(p);
    case SETSTRING:
        return new SetStringRecord(p);
    default:
        return nullptr;
    }
    return nullptr;
}