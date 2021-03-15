/* Copyright (c) 2004, 2016, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file ha_example.cc

  @brief
  The ha_example engine is a stubbed storage engine for example purposes only;
  it does nothing at this point. Its purpose is to provide a source
  code illustration of how to begin writing new storage engines; see also
  /storage/example/ha_example.h.

  @details
  ha_example will let you create/open/delete tables, but
  nothing further (for example, indexes are not supported nor can data
  be stored in the table). Use this example as a template for
  implementing the same functionality in your own storage engine. You
  can enable the example storage engine in your build by doing the
  following during your build process:<br> ./configure
  --with-example-storage-engine

  Once this is done, MySQL will let you create tables with:<br>
  CREATE TABLE <table name> (...) ENGINE=EXAMPLE;

  The example storage engine is set up to use table locks. It
  implements an example "SHARE" that is inserted into a hash by table
  name. You can use this to store information of state that any
  example handler object will be able to see when it is using that
  table.

  Please read the object definition in ha_example.h before reading the rest
  of this file.

  @note
  When you create an EXAMPLE table, the MySQL Server creates a table .frm
  (format) file in the database directory, using the table name as the file
  name as is customary with MySQL. No other files are created. To get an idea
  of what occurs, here is an example select that would do a scan of an entire
  table:

  @code
  ha_example::store_lock
  ha_example::external_lock
  ha_example::info
  ha_example::rnd_init
  ha_example::extra
  ENUM HA_EXTRA_CACHE        Cache record in HA_rrnd()
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::rnd_next
  ha_example::extra
  ENUM HA_EXTRA_NO_CACHE     End caching of records (def)
  ha_example::external_lock
  ha_example::extra
  ENUM HA_EXTRA_RESET        Reset database to after open
  @endcode

  Here you see that the example storage engine has 9 rows called before
  rnd_next signals that it has reached the end of its data. Also note that
  the table in question was already opened; had it not been open, a call to
  ha_example::open() would also have been necessary. Calls to
  ha_example::extra() are hints as to what will be occuring to the request.

  A Longer Example can be found called the "Skeleton Engine" which can be 
  found on TangentOrg. It has both an engine and a full build environment
  for building a pluggable storage engine.

  Happy coding!<br>
    -Brian
*/

#include "sql_class.h"           // MYSQL_HANDLERTON_INTERFACE_VERSION
#include "ha_example.h"
#include "probes_mysql.h"
#include "sql_plugin.h"

#include "page.h"
#include "FileMgr.h"
#include "LogMgr.h"
#include "Buffer.h"
#include "BufferMgr.h"
#include "LogRecord.h"
#include "Schema.h"
#include "Layout.h"
#include "RecordPage.h"
#include "Scan.h"
#include "TableMgr.h"
#include "MetadataMgr.h"
#include "Term.h"
#include "SelectScan.h"
#include "TablePlan.h"
#include "SelectPlan.h"
#include "ProjectPlan.h"
#include "ProductPlan.h"

#include <mysql/thread_pool_priv.h>
#include "Lexer.h"
#include "Parser.h"

static handler *example_create_handler(handlerton *hton,
                                       TABLE_SHARE *table, 
                                       MEM_ROOT *mem_root);

handlerton *example_hton;

/* Interface to mysqld, to check system tables supported by SE */
static const char* example_system_database();
static bool example_is_supported_system_table(const char *db,
                                      const char *table_name,
                                      bool is_sql_layer_system_table);

Example_share::Example_share()
{
  thr_lock_init(&lock);
}

//各种测试模块
void testFileMgr() {
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);

    BlockId* blk = BlockId::New("testfile", 2);
    int pos1 = 88;

    Page* p1 = new Page(fm->blockSize());
    p1->setString(pos1, "abcdefghijklm");
    int size = Page::maxLength(strlen("abcdefghijklm"));
    int pos2 = pos1 + size;
    p1->setInt(pos2, 345);
    fm->write(*blk, *p1);

    Page* p2 = new Page(fm->blockSize());
    fm->read(*blk, *p2);

    //三种都写入mysql 错误日志中
    sql_print_information("offset %d contains %d \n", pos2, p2->getInt(pos2));
    sql_print_information("offset %d contains %s \n", pos1, p2->getString(pos1).c_str());

//    fprintf(stderr, "offset %d contains %d \n", pos2, p2->getInt(pos2));;
//    fprintf(stderr, "offset %d contains %s \n", pos1, p2->getString(pos1).c_str());

//    std::cerr << "offset " << pos2 << " contains " << p2->getInt(pos2) << std::endl;
//    std::cerr << "offset " << pos1 << " contains " << p2->getString(pos1) << std::endl;

    delete fm;
    delete blk;
    delete p1;
    delete p2;
}


void printLogRecords(LogMgr *lm, const std::string& msg) {
    sql_print_information("%s \n",msg.c_str());
    LogIterator* iter = lm->iterator();
    while (iter->hasNext()) {
        std::unique_ptr<ByteBuffer> rec = iter->next();
        Page* p = Page::New(rec.get());
        std::string s = p->getString(0);
        int npos = Page::maxLength(s.length());
        int val = p->getInt(npos);
        sql_print_information("[ %s , %d ]",s.c_str(),val);

        p->extractBuffer();

        delete p;
    }
    sql_print_information("\n");
}

//创建日志记录<字符串，值>
ByteBuffer* createLogRecord(std::string s, int n) {
    int spos = 0;
    int npos = spos + Page::maxLength(s.length());
    ByteBuffer* b = ByteBuffer::New(npos + sizeof(int));
    Page* p = Page::New(b);
    p->setString(spos, s);
    p->setInt(npos, n);
    ByteBuffer* ret =  p->extractBuffer();
    delete p;
    return ret;
}

//
void createRecords(LogMgr * lm,int start, int end) {
    sql_print_information("Creating records: ");
    for (int i = start; i <= end; i++) {
        ByteBuffer* rec = createLogRecord("record" + std::to_string(i), i + 100);
        int lsn = lm->append(rec);
        sql_print_information("%d ",lsn);
        delete rec;
    }
    sql_print_information("\n");
}

void testLogMgr() {
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    printLogRecords(lm,"The initial empty log file:");  //print an empty log file
    sql_print_information("done\n");
    createRecords(lm,1, 35);
    printLogRecords(lm,"The log file now has these records:");
    createRecords(lm,36, 70);
    lm->flush(65);
    printLogRecords(lm,"The log file now has these records:");

    delete fm;
    delete lm;
}

void testBuffer(){
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 3);

    Buffer* buff1 = bm->pin(new BlockId("testfile", 1));
    assert(buff1);
    Page* p = buff1->contents();
    int n = p->getInt(80);
    p->setInt(80, n + 1);
    buff1->setModified(1, 0); //placeholder values
    sql_print_information("The new value is %d\n",(n + 1));
    bm->unpin(buff1);
    // One of these pins will flush buff1 to disk:
    Buffer* buff2 = bm->pin(new BlockId("testfile", 2));
    Buffer* buff3 = bm->pin(new BlockId("testfile", 3));
    Buffer* buff4 = bm->pin(new BlockId("testfile", 4));

    bm->unpin(buff2);
    buff2 = bm->pin(new BlockId("testfile", 1));
    Page* p2 = buff2->contents();
    p2->setInt(80, 9999);     // This modification
    buff2->setModified(1, 0); // won't get written to disk.

    delete fm;
    delete lm;
    delete bm;
}

void testBufferFile() {
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    BlockId* blk = new BlockId("testfile", 2);
    int pos1 = 88;

    Buffer* b1 = bm->pin(blk);
    Page* p1 = b1->contents();
    p1->setString(pos1, "abcdefghijklm");
    int size = Page::maxLength(strlen("abcdefghijklm"));
    int pos2 = pos1 + size;
    p1->setInt(pos2, 345);
    b1->setModified(1, 0);
    bm->unpin(b1);

    Buffer* b2 = bm->pin(blk);
    Page* p2 = b2->contents();
    sql_print_information("offset %d contains %d\n",pos2, p2->getInt(pos2));
    sql_print_information("offset %d contains %s\n",pos1,p2->getString(pos1).c_str());
    bm->unpin(b2);

    delete fm;
    delete lm;
    delete bm;
}

void testBufferMgr() {
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 3);

    std::vector<Buffer*> buff(6);
    buff[0] = bm->pin(new BlockId("testfile", 0));
    buff[1] = bm->pin(new BlockId("testfile", 1));
    buff[2] = bm->pin(new BlockId("testfile", 2));
    bm->unpin(buff[1]); 
    buff[1] = nullptr;
    buff[3] = bm->pin(new BlockId("testfile", 0)); // block 0 pinned twice
    buff[4] = bm->pin(new BlockId("testfile", 1)); // block 1 repinned
    sql_print_information("Available buffers: %d \n", bm->available());
    sql_print_information("Attempting to pin block 3... \n");
    buff[5] = bm->pin(new BlockId("testfile", 3)); // will not work; no buffers left
    if (buff[5] == nullptr) {
        sql_print_information("Exception: No available buffers\n");
    }
    
    bm->unpin(buff[2]); 
    buff[2] = nullptr;
    buff[5] = bm->pin(new BlockId("testfile", 3)); // now this works

    sql_print_information("Final Buffer Allocation:");
    for (int i = 0; i < buff.size(); i++) {
        Buffer* b = buff[i];
        if (b != nullptr)
            sql_print_information("buff[%d] pinned to block %s \n",i, b->block()->to_str().c_str());
    }

    delete fm;
    delete lm;
    delete bm;
}

void testPrintLogFile() {
    int blocksize = 400;
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);
    std::string filename = "simpledb.log";
    
    int lastblock = fm->length(filename) - 1;
    BlockId* blk = BlockId::New(filename, lastblock);
    Page* p = Page::New(fm->blockSize());
    fm->read(*blk, *p);
    LogIterator* iter = lm->iterator();
    while (iter->hasNext()) {
        std::unique_ptr<ByteBuffer> bytes = iter->next();
        LogRecord* rec = LogRecord::createLogRecord(bytes.get());
        if (rec) {
            sql_print_information("%s\n", rec->to_str().c_str());
        }
        else {
            sql_print_information("rec is nullptr \n");
        }
        
    }
}

void printValues(const std::string& msg, FileMgr* fm, BlockId* blk0, BlockId* blk1) {
    sql_print_information("%s\n",msg.c_str());
    Page* p0 = Page::New(fm->blockSize());
    Page* p1 = Page::New(fm->blockSize());
    fm->read(*blk0, *p0);
    fm->read(*blk1, *p1);
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        sql_print_information("%d ",p0->getInt(pos));
        sql_print_information("%d ",p1->getInt(pos));
        pos += sizeof(int);
    }
    sql_print_information("%s",p0->getString(30).c_str());
    sql_print_information("%s",p1->getString(30).c_str());
    sql_print_information("\n");
}

void initialize(FileMgr* fm, LogMgr* lm, BufferMgr* bm, BlockId* blk0, BlockId* blk1, LockTable* locktbl) {
    Transaction* tx1 = new Transaction(fm,lm,bm,locktbl);
    Transaction* tx2 = new Transaction(fm,lm,bm,locktbl);
    assert(tx1->pin(blk0));
    assert(tx2->pin(blk1));
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        assert(tx1->setInt(blk0, pos, pos, false) == TX_SUCC);
        assert(tx2->setInt(blk1, pos, pos, false) == TX_SUCC);
        pos += sizeof(int);
    }
    assert(tx1->setString(blk0, 30, "abc", false) == TX_SUCC);
    assert(tx2->setString(blk1, 30, "def", false) == TX_SUCC);
    tx1->commit();
    tx2->commit();
    printValues("After Initialization:",fm,blk0,blk1);
    delete tx1;
    delete tx2;
}

void modify(FileMgr* fm, LogMgr* lm, BufferMgr* bm, BlockId* blk0, BlockId* blk1, LockTable* locktbl) {
    Transaction* tx3 = new Transaction(fm, lm, bm,locktbl);
    Transaction* tx4 = new Transaction(fm, lm, bm,locktbl);
    assert(tx3->pin(blk0));
    assert(tx4->pin(blk1));
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        assert(tx3->setInt(blk0, pos, pos + 100, true) == TX_SUCC);
        assert(tx4->setInt(blk1, pos, pos + 100, true) == TX_SUCC);
        pos += sizeof(int);
    }
    assert(tx3->setString(blk0, 30, "uvw", true) == TX_SUCC);
    assert(tx4->setString(blk1, 30, "xyz", true) == TX_SUCC);
    bm->flushAll(3);
    bm->flushAll(4);
    printValues("After modification:", fm, blk0, blk1);

    tx3->rollback();
    printValues("After rollback:", fm, blk0, blk1);
    // tx4 stops here without committing or rolling back,
    // so all its changes should be undone during recovery.
    delete tx3;
    delete tx4;
}

void recover(FileMgr* fm, LogMgr* lm, BufferMgr* bm, BlockId* blk0, BlockId* blk1, LockTable* locktbl) {
    Transaction* tx = new Transaction(fm, lm, bm, locktbl);
    tx->recover();
    printValues("After recovery:",fm,blk0,blk1);
    delete tx;
}

void testRecovery() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);
    std::string filename = "simpledb.log";

    BlockId *blk0, *blk1;

    blk0 = BlockId::New("testfile", 0);
    blk1 = BlockId::New("testfile", 1);

    if (fm->length("testfile") == 0) {
        initialize(fm,lm,bm,blk0,blk1,locktbl);
        modify(fm, lm, bm, blk0, blk1,locktbl);
    }
    else {
        recover(fm, lm, bm, blk0, blk1, locktbl);
    }

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testTx() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx1 = new Transaction(fm, lm, bm,locktbl);
    BlockId* blk = BlockId::New("testfile", 1);
    assert(tx1->pin(blk));
    // The block initially contains unknown bytes,
    // so don't log those values here.
    assert(tx1->setInt(blk, 80, 1, false) == TX_SUCC);
    assert(tx1->setString(blk, 40, "one", false) == TX_SUCC);
    tx1->commit();

    Transaction* tx2 = new Transaction(fm, lm, bm,locktbl);
    assert(tx2->pin(blk));
    int ival;
    assert(tx2->getInt(blk, 80,ival) == TX_SUCC);
    std::string sval;
    assert(tx2->getString(blk, 40,sval) == TX_SUCC);
    sql_print_information("initial value at location 80 = %d\n",ival);
    sql_print_information("initial value at location 40 = %s\n",sval.c_str());
    int newival = ival + 1;
    std::string newsval = sval + "!";
    assert(tx2->setInt(blk, 80, newival, true) == TX_SUCC);
    assert(tx2->setString(blk, 40, newsval, true) == TX_SUCC);
    tx2->commit();
    Transaction* tx3 = new Transaction(fm, lm, bm, locktbl);
    assert(tx3->pin(blk));
    int i80;
    assert(tx3->getInt(blk, 80,i80) == TX_SUCC);
    std::string i40;
    assert(tx3->getString(blk, 40,i40) == TX_SUCC);
    sql_print_information("new value at location 80 = %d\n",i80);
    sql_print_information("new value at location 40 = %s\n",i40.c_str());
    assert(tx3->setInt(blk, 80, 9999, true) == TX_SUCC);
    assert(tx3->getInt(blk, 80, i80) == TX_SUCC);
    sql_print_information("pre-rollback value at location 80 = %d\n",i80);
    tx3->rollback();

    Transaction* tx4 = new Transaction(fm, lm, bm, locktbl);
    assert(tx4->pin(blk));

    assert(tx4->getInt(blk, 80,i80) == TX_SUCC);
    sql_print_information("post-rollback at location 80 = %d\n", i80);
    tx4->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
    delete tx1;
    delete tx2;
    delete tx3;
    delete tx4;
}

struct ConcurrencyArgs {
    LockTable* locktbl;
    FileMgr* fm;
    LogMgr* lm;
    BufferMgr* bm;
};

void* thread_a_body(void* p) {
    ConcurrencyArgs* args = (ConcurrencyArgs*)p;
    Transaction* txA = new Transaction(args->fm, args->lm, args->bm,args->locktbl);
    BlockId* blk1 = BlockId::New("testfile", 1);
    BlockId* blk2 = BlockId::New("testfile", 2);
    if (!txA->pin(blk1)) {
        sql_print_error("txA pin blk1 failed\n");
        return nullptr;
    }
    if (!txA->pin(blk2)) {
        sql_print_error("txA pin blk2 failed\n");
        return nullptr;
    }
    sql_print_information("Tx A: request slock 1\n");
    int i0;
    TransStatus ret = txA->getInt(blk1, 0, i0);
    if (ret != TX_SUCC) {
        sql_print_error("txA getInt from blk1 failed\n");
        return nullptr;
    }
    sql_print_information("Tx A: receive slock 1\n");
    sleep(2);
    sql_print_information("Tx A: request slock 2\n");
    ret = txA->getInt(blk2, 0,i0);
    if (ret != TX_SUCC) {
        sql_print_error("txA getInt from blk2 failed\n");
        return nullptr;
    }
    sql_print_information("Tx A: receive slock 2\n");
    txA->commit();
    sql_print_information("Tx A: commit\n");
    delete txA;
}

void* thread_b_body(void* p) {
    ConcurrencyArgs* args = (ConcurrencyArgs*)p;
    Transaction* txB = new Transaction(args->fm, args->lm, args->bm, args->locktbl);
    BlockId* blk1 = BlockId::New("testfile", 1);
    BlockId* blk2 = BlockId::New("testfile", 2);
    if (!txB->pin(blk1)) {
        sql_print_error("txB pin blk1 failed\n");
        return nullptr;
    }
    if (!txB->pin(blk2)) {
        sql_print_error("txB pin blk2 failed\n");
        return nullptr;
    }
    sql_print_information("Tx B: request xlock 2\n");
    TransStatus ret = txB->setInt(blk2, 0, 0, false);
    if (ret != TX_SUCC) {
        sql_print_error("txB setInt in blk2 failed\n");
        return nullptr;
    }
    sql_print_information("Tx B: receive xlock 2\n");
    sleep(2);
    sql_print_information("Tx B: request slock 1\n");
    int i0;
    ret = txB->getInt(blk1, 0,i0);
    if (ret != TX_SUCC) {
        sql_print_error("txB getInt from blk1 failed\n");
        return nullptr;
    }
    sql_print_information("Tx B: receive slock 1\n");
    txB->commit();
    sql_print_information("Tx B: commit\n");
}

void* thread_c_body(void* p) {
    ConcurrencyArgs* args = (ConcurrencyArgs*)p;
    Transaction* txC = new Transaction(args->fm, args->lm, args->bm, args->locktbl);
    BlockId* blk1 = BlockId::New("testfile", 1);
    BlockId* blk2 = BlockId::New("testfile", 2);
    if(!txC->pin(blk1)) {
        sql_print_error("txC pin blk1 failed\n");
        return nullptr;
    }
    if(!txC->pin(blk2)) {
        sql_print_error("txC pin blk1 failed\n");
        return nullptr;
    }
    sleep(1);
    sql_print_information("Tx C: request xlock 1\n");
    TransStatus ret = txC->setInt(blk1, 0, 0, false);
    if (ret != TX_SUCC) {
        sql_print_error("txC setInt in blk1 failed\n");
        return nullptr;
    }
    sql_print_information("Tx C: receive xlock 1\n");
    sleep(1);
    sql_print_information("Tx C: request slock 2\n");
    int i0;
    ret = txC->getInt(blk2, 0,i0);
    if (ret != TX_SUCC) {
        sql_print_error("txC getInt from blk2 failed\n");
        return nullptr;
    }
    sql_print_information("Tx C: receive slock 2\n");
    txC->commit();
    sql_print_information("Tx C: commit\n");
}

void testConcurrency() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    ConcurrencyArgs args;
    args.locktbl = locktbl;
    args.fm = fm;
    args.lm = lm;
    args.bm = bm;

    my_thread_attr_t a_attr;
    my_thread_attr_t b_attr;
    my_thread_attr_t c_attr;

    my_thread_attr_init(&a_attr);
    my_thread_attr_setdetachstate(&a_attr, MY_THREAD_CREATE_JOINABLE);

    my_thread_attr_init(&b_attr);
    my_thread_attr_setdetachstate(&b_attr, MY_THREAD_CREATE_JOINABLE);

    my_thread_attr_init(&c_attr);
    my_thread_attr_setdetachstate(&c_attr, MY_THREAD_CREATE_JOINABLE);
    
    my_thread_handle thread_a;
    my_thread_handle thread_b;
    my_thread_handle thread_c;

    if (my_thread_create(&thread_a, &a_attr, thread_a_body,&args) != 0)
    {
        fprintf(stderr, "Could not create thread a!\n");
        exit(0);
    }

    if (my_thread_create(&thread_b, &b_attr, thread_b_body, &args) != 0)
    {
        fprintf(stderr, "Could not create thread b!\n");
        exit(0);
    }

    if (my_thread_create(&thread_c, &c_attr, thread_c_body, &args) != 0)
    {
        fprintf(stderr, "Could not create thread c!\n");
        exit(0);
    }

    void* dummy;
    my_thread_join(&thread_a, &dummy);
    my_thread_join(&thread_b, &dummy);
    my_thread_join(&thread_c, &dummy);

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testLayout() {
    Schema sch;
    sch.addIntField("A");
    sch.addStringField("B", 9);
    Layout layout(sch);
    for (const std::string fldname : layout.schema().fields()) {
        int offset = layout.offset(fldname);
        sql_print_information("%s has offset %d\n",fldname.c_str(),offset);
    }
}

void testRecord() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm,lm,bm,locktbl);

    Schema* sch = new Schema();
    sch->addIntField("A");
    sch->addStringField("B", 9);
    Layout* layout = new Layout(*sch);
    for (const std::string fldname : layout->schema().fields()) {
        int offset = layout->offset(fldname);
        sql_print_information("%s has offset %d\n",fldname.c_str(),offset);
    }
    BlockId* blk;
    assert(tx->append("testfile",blk)  == TX_SUCC);
    assert(tx->pin(blk));
    RecordPage* rp = new RecordPage(tx, blk, layout);
    assert(rp->format());

    sql_print_information("Filling the page with random records.\n");
    int slot = rp->insertAfter(-1);
    assert(slot >= 0);
    while (slot >= 0) {
        int n = 1 + rand() % 50;
        assert(rp->setInt(slot, "A", n));
        assert(rp->setString(slot, "B", "rec" + std::to_string(n)));
        sql_print_information("inserting into slot %d: { %d, rec%d}\n", slot,n,n);
        slot = rp->insertAfter(slot);
    }

    sql_print_information("Deleting these records, whose A-values are less than 25.\n");
    int count = 0;
    slot = rp->nextAfter(-1);
    while (slot >= 0) {
        int a;
        assert(rp->getInt(slot, "A",a));
        std::string b;
        assert(rp->getString(slot, "B",b));
        if (a < 25) {
            count++;
            sql_print_information("slot %d: {%d, %s}\n", slot,a,b.c_str());
            assert(rp->remove(slot));
        }
        slot = rp->nextAfter(slot);
    }
    sql_print_information("%d values under 25 were deleted.\n", count);

    sql_print_information("Here are the remaining records.\n");
    slot = rp->nextAfter(-1);
    while (slot >= 0) {
        int a;
        assert(rp->getInt(slot, "A",a));
        std::string b;
        assert(rp->getString(slot, "B",b));
        sql_print_information("slot %d: {%d, %s}\n",slot,a,b.c_str());
        slot = rp->nextAfter(slot);
    }
    tx->unpin(blk);
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testTableScan() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);
    
    Schema* sch = new Schema();
    sch->addIntField("A");
    sch->addStringField("B", 9);
    Layout* layout = new Layout(*sch);
    for (const std::string& fldname : layout->schema().fields()) {
        int offset = layout->offset(fldname);
        sql_print_information("%s has offset %d\n",fldname.c_str(),offset);
    }

    sql_print_information("Filling the table with 50 random records.\n");
    TableScan* ts = new TableScan(tx, "T", layout);
    for (int i = 0; i < 50; i++) {
        ts->insert();
        int n = rand() % 50;
        ts->setInt("A", n);
        ts->setString("B", "rec" + std::to_string(n));
        sql_print_information("inserting into slot %s: {%d, rec%d}\n",
            ts->getRid().toString().c_str(),
            n,
            n);
    }

    sql_print_information("Deleting these records, whose A-values are less than 25.\n");
    int count = 0;
    ts->beforeFirst();
    while (ts->next()) {
        int a = ts->getInt("A");
        std::string b = ts->getString("B");
        if (a < 25) {
            count++;
            sql_print_information("slot %s: {%d, %s}\n",
                ts->getRid().toString().c_str(),
                a,
                b.c_str()
                );
            ts->remove();
        }
    }
    sql_print_information("%d values under 10 were deleted.\n",count);

    sql_print_information("Here are the remaining records.\n");
    ts->beforeFirst();
    while (ts->next()) {
        int a = ts->getInt("A");
        std::string b = ts->getString("B");
        sql_print_information("slot %s: {%d, %s}\n",
            ts->getRid().toString().c_str(),
            a,
            b.c_str()
            );
    }
    ts->close();
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testTableMgr() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    TableMgr* tm = new TableMgr(true, tx);

    Schema sch;
    sch.addIntField("A");
    sch.addStringField("B", 9);
    tm->createTable("MyTable", &sch, tx);

    Layout* layout = tm->getLayout("MyTable", tx);
    int size = layout->slotSize();
    Schema& sch2 = layout->schema();
    sql_print_information("MyTable has slot size %d\n",size);
    sql_print_information("Its fields are:\n");
    for (const std::string& fldname : sch2.fields()) {
        std::string type;
        if (sch2.type(fldname) == INTEGER)
            type = "int";
        else {
            int strlen = sch2.length(fldname);
            type = "varchar(" + std::to_string(strlen) + ")";
        }
        sql_print_information("%s: %s\n", fldname.c_str(), type.c_str());
    }
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testMetadataMgr() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    MetadataMgr* mdm = new MetadataMgr(true, tx);

    Schema sch;
    sch.addIntField("A");
    sch.addStringField("B", 9);

    // Part 1: Table Metadata
    mdm->createTable("MyTable", sch, tx);
    Layout* layout = mdm->getLayout("MyTable", tx);
    int size = layout->slotSize();
    Schema& sch2 = layout->schema();
    sql_print_information("MyTable has slot size %d\n",size);
    sql_print_information("Its fields are:\n");
    for (const std::string& fldname : sch2.fields()) {
        std::string type;
        if (sch2.type(fldname) == INTEGER)
            type = "int";
        else {
            int strlen = sch2.length(fldname);
            type = "varchar(" + std::to_string(strlen) + ")";
        }
        sql_print_information("%s:%s ", fldname.c_str(),type.c_str());
    }

    // Part 2: Statistics Metadata
    TableScan ts(tx, "MyTable", layout);
    for (int i = 0; i < 50; i++) {
        ts.insert();
        int n = rand() % 50;
        ts.setInt("A", n);
        ts.setString("B", "rec" + std::to_string(n));
    }
    StatInfo si = mdm->getStatInfo("MyTable", layout, tx);
    sql_print_information("B(MyTable) = %d\n", si.blocksAccessed());
    sql_print_information("R(MyTable) = %d\n", si.recordsOutput());
    sql_print_information("V(MyTable,A) = %d\n", si.distinctValues("A"));
    sql_print_information("V(MyTable,B) = %d\n", si.distinctValues("B"));

    // Part 3: View Metadata     
    std::string viewdef = "select B from MyTable where A = 1";
    mdm->createView("viewA", viewdef, tx);
    std::string v = mdm->getViewDef("viewA", tx);
    sql_print_information("View def = %s\n", v.c_str());

    // Part 4: Index Metadata
    mdm->createIndex("indexA", "MyTable", "A", tx);
    mdm->createIndex("indexB", "MyTable", "B", tx);
    std::unordered_map<std::string, IndexInfo*> idxmap = mdm->getIndexInfo("MyTable", tx);

    IndexInfo* ii = idxmap["A"];
    sql_print_information("B(indexA) = %d\n" ,ii->blocksAccessed());
    sql_print_information("R(indexA) = %d\n" ,ii->recordsOutput());
    sql_print_information("V(indexA,A) = %d\n",ii->distinctValues("A"));
    sql_print_information("V(indexA,B) = %d\n", ii->distinctValues("B"));

    ii = idxmap["B"];
    sql_print_information("B(indexB) = %d\n",ii->blocksAccessed());
    sql_print_information("R(indexB) = %d\n", ii->recordsOutput());
    sql_print_information("V(indexB,A) = %d\n", ii->distinctValues("A"));
    sql_print_information("V(indexB,B) = %d\n", ii->distinctValues("B"));
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
    delete mdm;
}

void testCatalog() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    TableMgr* tm = new TableMgr(false, tx);
    Layout* tcatLayout = tm->getLayout("tblcat", tx);

    sql_print_information("Here are all the tables and their lengths.\n");
    TableScan* ts = new TableScan(tx, "tblcat", tcatLayout);
    while (ts->next()) {
        std::string tname = ts->getString("tblname");
        int slotsize = ts->getInt("slotsize");
        sql_print_information( "%s %d\n", tname.c_str(),slotsize);
    }
    ts->close();

    delete ts;
    ts = nullptr;

    sql_print_information("\nHere are the fields for each table and their offsets");
    Layout* fcatLayout = tm->getLayout("fldcat", tx);
    ts = new TableScan(tx, "fldcat", fcatLayout);
    while (ts->next()) {
        std::string tname = ts->getString("tblname");
        std::string fname = ts->getString("fldname");
        int offset = ts->getInt("offset");
        sql_print_information("%s %s %d\n", tname.c_str(), fname.c_str(),offset);
    }
    ts->close();

    delete ts;

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
    delete tm;
}

void testProduct() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    Schema sch1;
    sch1.addIntField("A");
    sch1.addStringField("B", 9);
    Layout* layout1 = new Layout(sch1);
    TableScan* ts1 = new TableScan(tx, "T1", layout1);

    Schema sch2;
    sch2.addIntField("C");
    sch2.addStringField("D", 9);
    Layout* layout2 = new Layout(sch2);
    TableScan* ts2 = new TableScan(tx, "T2", layout2);

    ts1->beforeFirst();
    int n = 200;
    sql_print_information("Inserting %d records into T1.\n", n);
    for (int i = 0; i < n; i++) {
        ts1->insert();
        ts1->setInt("A", i);
        ts1->setString("B", "aaa" + std::to_string(i));
    }
    ts1->close();

    ts2->beforeFirst();
    sql_print_information("Inserting %d records into T2.\n",n);
    for (int i = 0; i < n; i++) {
        ts2->insert();
        ts2->setInt("C", n - i - 1);
        ts2->setString("D", "bbb" + std::to_string(n - i - 1));
    }
    ts2->close();

    Scan* s1 = new TableScan(tx, "T1", layout1);
    Scan* s2 = new TableScan(tx, "T2", layout2);
    Scan* s3 = new ProductScan(s1, s2);
    while (s3->next())
        sql_print_information("%s\n",s3->getString("B").c_str());
    s3->close();
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testScan1() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    Schema sch1;
    sch1.addIntField("A");
    sch1.addStringField("B", 9);
    Layout* layout = new Layout(sch1);
    UpdateScan* s1 = new TableScan(tx, "T", layout);

    s1->beforeFirst();
    int n = 200;
    sql_print_information("Inserting %d random records.\n",n);
    for (int i = 0; i < n; i++) {
        s1->insert();
        int k = rand() % 50;
        s1->setInt("A", k);
        s1->setString("B", "rec" + std::to_string(k));
    }
    s1->close();

    Scan* s2 = new TableScan(tx, "T", layout);
    // selecting all records where A=10
    Constant* c = new Constant(10);
    Term* t = new Term(new Expression("A"), new Expression(c));
    Predicate* pred = new Predicate(t);
    sql_print_information("The predicate is %s\n",pred->toString().c_str());
    Scan* s3 = new SelectScan(s2, pred);
    std::list<std::string> fields = { "B" };
    Scan* s4 = new ProjectScan(s3, fields);
    while (s4->next())
        sql_print_information("%s\n",s4->getString("B").c_str());
    s4->close();
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void testScan2() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);

    Schema sch1;
    sch1.addIntField("A");
    sch1.addStringField("B", 9);
    Layout* layout1 = new Layout(sch1);
    UpdateScan* us1 = new TableScan(tx, "T1", layout1);
    us1->beforeFirst();
    int n = 200;
    sql_print_information("Inserting %d records into T1.\n",n);
    for (int i = 0; i < n; i++) {
        us1->insert();
        us1->setInt("A", i);
        us1->setString("B", "bbb" + std::to_string(i));
    }
    us1->close();

    Schema sch2;
    sch2.addIntField("C");
    sch2.addStringField("D", 9);
    Layout* layout2 = new Layout(sch2);
    UpdateScan* us2 = new TableScan(tx, "T2", layout2);
    us2->beforeFirst();
    sql_print_information("Inserting %d records into T2.\n",n);
    for (int i = 0; i < n; i++) {
        us2->insert();
        us2->setInt("C", n - i - 1);
        us2->setString("D", "ddd" + std::to_string(n - i - 1));
    }
    us2->close();

    Scan* s1 = new TableScan(tx, "T1", layout1);
    Scan* s2 = new TableScan(tx, "T2", layout2);
    Scan* s3 = new ProductScan(s1, s2);
    // selecting all records where A=C
    Term* t = new Term(new Expression("A"), new Expression("C"));
    Predicate* pred = new Predicate(t);
    sql_print_information("The predicate is %s\n", pred->toString().c_str());
    Scan* s4 = new SelectScan(s3, pred);

    // projecting on [B,D]
    std::list<std::string> c = { "B", "D" };
    Scan* s5 = new ProjectScan(s4, c);
    while (s5->next())
        sql_print_information("%s %s" , s5->getString("B").c_str(), s5->getString("D").c_str());
    s5->close();
    tx->commit();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
}

void printStats(int n, Plan* p) {
    sql_print_information("Here are the stats for plan p %d",n);
    sql_print_information("\tR(p%d): %d",n,p->recordsOutput());
    sql_print_information("\tB(p%d): %d",n,p->blocksAccessed());
    sql_print_information("\n");
}

struct student_info {
    int sid;
    const char* sname;
    int gradyear;
    int majorid;
};

struct dept_info {
    int did;
    const char* dname;
};

void createStudentTable(MetadataMgr* mdm, Transaction* tx) {
    Schema sch;
    sch.addIntField("sid");
    sch.addStringField("sname", 16);
    sch.addIntField("gradyear");
    sch.addIntField("majorid");

    std::vector<student_info> table = {
        {1,"joe",2004,10},
        {2,"amy",2004,20},
        {3,"max",2005,10},
        {4,"sue",2005,20},
        {5,"bob",2003,30},
        {6,"kim",2001,20},
        {7,"art",2004,30},
        {8,"pat",2001,20},
        {9,"lee",2004,10},
    };
    mdm->createTable("student", sch, tx);
    Layout* layout = mdm->getLayout("student", tx);
    UpdateScan* us = new TableScan(tx, "student", layout);
    us->beforeFirst();
    sql_print_information("Inserting %d records into student.\n", table.size());
    for (int i = 0; i < table.size(); i++) {
        student_info& info = table[i];
        us->insert();
        us->setInt("sid", info.sid);
        us->setString("sname", info.sname);
        us->setInt("gradyear", info.gradyear);
        us->setInt("majorid", info.majorid);
    }
    us->close();
    mdm->getStatInfo("student", layout, tx);
}

void createDeptTable(MetadataMgr* mdm, Transaction* tx) {
    Schema sch;
    sch.addIntField("did");
    sch.addStringField("dname", 16);

    std::vector<dept_info> table = {
        {10,"compsci"},
        {20,"math"},
        {30,"drama"},
    };
    mdm->createTable("dept", sch, tx);
    Layout* layout = mdm->getLayout("dept", tx);
    UpdateScan* us = new TableScan(tx, "dept", layout);
    us->beforeFirst();
    sql_print_information("Inserting %d records into dept.\n", table.size());
    for (int i = 0; i < table.size(); i++) {
        dept_info& info = table[i];
        us->insert();
        us->setInt("did", info.did);
        us->setString("dname", info.dname);
    }
    us->close();
    mdm->getStatInfo("dept", layout, tx);
}

void testSingleTablePlan() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);
    MetadataMgr* mdm = new MetadataMgr(true, tx);

    createStudentTable(mdm, tx);    

    //the STUDENT node
    Plan* p1 = new TablePlan(tx, "student", mdm);

    // the Select node for "major = 10"
    Term* t = new Term(new Expression("majorid"), new Expression(new Constant(10)));
    Predicate* pred = new Predicate(t);
    Plan* p2 = new SelectPlan(p1, pred);

    // the Select node for "gradyear = 2020"
    Term* t2 = new Term(new Expression("gradyear"), new Expression(new Constant(2020)));
    Predicate* pred2 = new Predicate(t2);
    Plan* p3 = new SelectPlan(p2, pred2);

    // the Project node
    std::list<std::string> c = { "sname", "majorid", "gradyear" };
    Plan* p4 = new ProjectPlan(p3, c);

    // Look at R(p) and B(p) for each plan p.
    printStats(1, p1); printStats(2, p2); printStats(3, p3); printStats(4, p4);

    // Change p2 to be p2, p3, or p4 to see the other scans in action.
    // Changing p2 to p4 will throw an exception because SID is not in the projection list.
    Scan* s = p2->open();
    while (s->next())
        sql_print_information("%d %s %d %d\n",
            s->getInt("sid"), s->getString("sname").c_str(),
            s->getInt("majorid"), s->getInt("gradyear"));
    s->close();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
    delete mdm;
}

void testMultiTablePlan() {
    int blocksize = 400;
    LockTable* locktbl = new LockTable();
    FileMgr* fm = FileMgr::New(blocksize);
    LogMgr* lm = LogMgr::New(fm, "simpledb.log");
    BufferMgr* bm = BufferMgr::New(fm, lm, 8);

    Transaction* tx = new Transaction(fm, lm, bm, locktbl);
    MetadataMgr* mdm = new MetadataMgr(true, tx);

    createStudentTable(mdm, tx);

    createDeptTable(mdm,tx);

    //the STUDENT node
    Plan* p1 = new TablePlan(tx, "student", mdm);
#if 0    
    Scan* s1 = p1->open();
    while (s1->next()) {
        sql_print_information("++++%d %s %d %d\n",
            s1->getInt("sid"), s1->getString("sname").c_str(),
            s1->getInt("majorid"), s1->getInt("gradyear"));
    }
    s1->close();
#endif

    //the DEPT node
    Plan* p2 = new TablePlan(tx, "dept", mdm);
#if 0
    Scan* s2 = p2->open();
    while (s2->next()) {
        sql_print_information("----%d %s\n",
            s2->getInt("did"), s2->getString("dname").c_str());
    }
    s2->close();
#endif

    //the Product node for student x dept
    Plan* p3 = new ProductPlan(p1, p2);

    // the Select node for "majorid = did"
    Term* t = new Term(new Expression("majorid"), new Expression("did"));
    Predicate* pred = new Predicate(t);
    Plan* p4 = new SelectPlan(p3, pred);

    // Look at R(p) and B(p) for each plan p.
    printStats(1, p1); printStats(2, p2); printStats(3, p3); printStats(4, p4);

    // Change p3 to be p4 to see the select scan in action.
    Scan* s = p3->open();
    while (s->next())
        sql_print_information("%s %s\n", 
            s->getString("sname").c_str(), 
            s->getString("dname").c_str());
    s->close();

    delete fm;
    delete lm;
    delete bm;
    delete locktbl;
    delete mdm;
}

void testLexer() {
    //"id = c" or "c = id"
    const char* inputs[3] = {
        "wo = 124",
        "122 = wx",
        "xxx = 10"
    };
    for (auto line : inputs) {
        Lexer lex(line);
        std::string x; int y;
        if (lex.matchId()) {
            x = lex.eatId();
            lex.eatDelim('=');
            y = lex.eatIntConstant();
        }
        else {
            y = lex.eatIntConstant();
            lex.eatDelim('=');
            x = lex.eatId();
        }
        sql_print_information("%s",(x + " equals " + std::to_string(y)).c_str());
    }
}

void testParser() {
    sql_print_information("Enter an SQL statement: ");
    std::string inputs[] = {
        "create view course_id_title_view as select cid,title from course",
        "create table course(cid INT,title VARCHAR(16),deptid INT)",
        "select sid,sname,gradyear,majorid from student,dept where majorid = did",
        "insert into student (sid,sname) values (123,'123')",
        "delete from student where sid = 124 and sname = '124'",
        "update student set sname = '125' where sid = 125",
        
        
        "create index course_id on course(cid)"
    };
    for (auto line : inputs) {
        Parser* p = new Parser(line);
        if (line.find("select") == 0) {
           QueryData *q =  p->query();
           sql_print_information("%s", q->toString().c_str());
        }
        else {
            ParserData* pd = p->updateCmd();
            sql_print_information("%s", pd->toString().c_str());
        }
        delete p;
    }
}

#ifdef HAVE_PSI_INTERFACE
static PSI_mutex_key adbe_logMgr_mutex;

static PSI_mutex_info all_adbe_mutexes[] =
{
  { &adbe_logMgr_mutex, "adbe_logMgr", PSI_FLAG_GLOBAL}
};

//static PSI_memory_info all_blackhole_memory[] =
//{
//  { &bh_key_memory_blackhole_share, "blackhole_share", 0}
//};

void init_adbe_psi_keys()
{
    const char* category = "adbe";
    int count;

    count = array_elements(all_adbe_mutexes);
    mysql_mutex_register(category, all_adbe_mutexes, count);

//    count = array_elements(all_blackhole_memory);
//    mysql_memory_register(category, all_blackhole_memory, count);
}
#endif

static int example_init_func(void *p)
{
  DBUG_ENTER("example_init_func");

#ifdef HAVE_PSI_INTERFACE
  init_adbe_psi_keys();
#endif

  example_hton= (handlerton *)p;
  example_hton->state=                     SHOW_OPTION_YES;
  example_hton->create=                    example_create_handler;
  example_hton->flags=                     HTON_CAN_RECREATE;
  example_hton->system_database=   example_system_database;
  example_hton->is_supported_system_table= example_is_supported_system_table;

  Transaction::init();

  //执行测试模块
  //testFileMgr();
  //testLogMgr();
  //testBuffer();
  //testBufferFile();
  //testBufferMgr();
  //testRecovery();
  //testTx();
  //testConcurrency();
  //testPrintLogFile();
  //testLayout();
  //testRecord();
  //testTableScan();
  //testTableMgr();
  //testMetadataMgr();
  //testCatalog();
  //testProduct();
  //testScan1();
  //testScan2();
  //testSingleTablePlan();
  //testMultiTablePlan();
  //testLexer();
  testParser();

  Transaction::deinit();
  DBUG_RETURN(0);
}

static int adbe_fini(void* p)
{
    //my_hash_free(&blackhole_open_tables);

    return 0;
}



/**
  @brief
  Example of simple lock controls. The "share" it creates is a
  structure we will pass to each example handler. Do you have to have
  one of these? Well, you have pieces that are used for locking, and
  they are needed to function.
*/

Example_share *ha_example::get_share()
{
  Example_share *tmp_share;

  DBUG_ENTER("ha_example::get_share()");

  lock_shared_ha_data();
  if (!(tmp_share= static_cast<Example_share*>(get_ha_share_ptr())))
  {
    tmp_share= new Example_share;
    if (!tmp_share)
      goto err;

    set_ha_share_ptr(static_cast<Handler_share*>(tmp_share));
  }
err:
  unlock_shared_ha_data();
  DBUG_RETURN(tmp_share);
}


static handler* example_create_handler(handlerton *hton,
                                       TABLE_SHARE *table, 
                                       MEM_ROOT *mem_root)
{
  return new (mem_root) ha_example(hton, table);
}

ha_example::ha_example(handlerton *hton, TABLE_SHARE *table_arg)
  :handler(hton, table_arg)
{}


/**
  @brief
  If frm_error() is called then we will use this to determine
  the file extensions that exist for the storage engine. This is also
  used by the default rename_table and delete_table method in
  handler.cc.

  For engines that have two file name extentions (separate meta/index file
  and data file), the order of elements is relevant. First element of engine
  file name extentions array should be meta/index file extention. Second
  element - data file extention. This order is assumed by
  prepare_for_repair() when REPAIR TABLE ... USE_FRM is issued.

  @see
  rename_table method in handler.cc and
  delete_table method in handler.cc
*/

static const char *ha_example_exts[] = {
  NullS
};

const char **ha_example::bas_ext() const
{
  return ha_example_exts;
}

/*
  Following handler function provides access to
  system database specific to SE. This interface
  is optional, so every SE need not implement it.
*/
const char* ha_example_system_database= NULL;
const char* example_system_database()
{
  return ha_example_system_database;
}

/*
  List of all system tables specific to the SE.
  Array element would look like below,
     { "<database_name>", "<system table name>" },
  The last element MUST be,
     { (const char*)NULL, (const char*)NULL }

  This array is optional, so every SE need not implement it.
*/
static st_handler_tablename ha_example_system_tables[]= {
  {(const char*)NULL, (const char*)NULL}
};

/**
  @brief Check if the given db.tablename is a system table for this SE.

  @param db                         Database name to check.
  @param table_name                 table name to check.
  @param is_sql_layer_system_table  if the supplied db.table_name is a SQL
                                    layer system table.

  @return
    @retval TRUE   Given db.table_name is supported system table.
    @retval FALSE  Given db.table_name is not a supported system table.
*/
static bool example_is_supported_system_table(const char *db,
                                              const char *table_name,
                                              bool is_sql_layer_system_table)
{
  st_handler_tablename *systab;

  // Does this SE support "ALL" SQL layer system tables ?
  if (is_sql_layer_system_table)
    return false;

  // Check if this is SE layer system tables
  systab= ha_example_system_tables;
  while (systab && systab->db)
  {
    if (systab->db == db &&
        strcmp(systab->tablename, table_name) == 0)
      return true;
    systab++;
  }

  return false;
}


/**
  @brief
  Used for opening tables. The name will be the name of the file.

  @details
  A table is opened when it needs to be opened; e.g. when a request comes in
  for a SELECT on the table (tables are not open and closed for each request,
  they are cached).

  Called from handler.cc by handler::ha_open(). The server opens all tables by
  calling ha_open() which then calls the handler specific open().

  @see
  handler::ha_open() in handler.cc
*/

int ha_example::open(const char *name, int mode, uint test_if_locked)
{
  DBUG_ENTER("ha_example::open");

  if (!(share = get_share()))
    DBUG_RETURN(1);
  thr_lock_data_init(&share->lock,&lock,NULL);

  DBUG_RETURN(0);
}


/**
  @brief
  Closes a table.

  @details
  Called from sql_base.cc, sql_select.cc, and table.cc. In sql_select.cc it is
  only used to close up temporary tables or during the process where a
  temporary table is converted over to being a myisam table.

  For sql_base.cc look at close_data_tables().

  @see
  sql_base.cc, sql_select.cc and table.cc
*/

int ha_example::close(void)
{
  DBUG_ENTER("ha_example::close");
  DBUG_RETURN(0);
}


/**
  @brief
  write_row() inserts a row. No extra() hint is given currently if a bulk load
  is happening. buf() is a byte array of data. You can use the field
  information to extract the data from the native byte array type.

  @details
  Example of this would be:
  @code
  for (Field **field=table->field ; *field ; field++)
  {
    ...
  }
  @endcode

  See ha_tina.cc for an example of extracting all of the data as strings.
  ha_berekly.cc has an example of how to store it intact by "packing" it
  for ha_berkeley's own native storage type.

  See the note for update_row() on auto_increments. This case also applies to
  write_row().

  Called from item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc, and sql_update.cc.

  @see
  item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc and sql_update.cc
*/

int ha_example::write_row(uchar *buf)
{
  DBUG_ENTER("ha_example::write_row");
  /*
    Example of a successful write_row. We don't store the data
    anywhere; they are thrown away. A real implementation will
    probably need to do something with 'buf'. We report a success
    here, to pretend that the insert was successful.
  */
  DBUG_RETURN(0);
}


/**
  @brief
  Yes, update_row() does what you expect, it updates a row. old_data will have
  the previous row record in it, while new_data will have the newest data in it.
  Keep in mind that the server can do updates based on ordering if an ORDER BY
  clause was used. Consecutive ordering is not guaranteed.

  @details
  Currently new_data will not have an updated auto_increament record. You can
  do this for example by doing:

  @code

  if (table->next_number_field && record == table->record[0])
    update_auto_increment();

  @endcode

  Called from sql_select.cc, sql_acl.cc, sql_update.cc, and sql_insert.cc.

  @see
  sql_select.cc, sql_acl.cc, sql_update.cc and sql_insert.cc
*/
int ha_example::update_row(const uchar *old_data, uchar *new_data)
{

  DBUG_ENTER("ha_example::update_row");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  This will delete a row. buf will contain a copy of the row to be deleted.
  The server will call this right after the current row has been called (from
  either a previous rnd_nexT() or index call).

  @details
  If you keep a pointer to the last row or can access a primary key it will
  make doing the deletion quite a bit easier. Keep in mind that the server does
  not guarantee consecutive deletions. ORDER BY clauses can be used.

  Called in sql_acl.cc and sql_udf.cc to manage internal table
  information.  Called in sql_delete.cc, sql_insert.cc, and
  sql_select.cc. In sql_select it is used for removing duplicates
  while in insert it is used for REPLACE calls.

  @see
  sql_acl.cc, sql_udf.cc, sql_delete.cc, sql_insert.cc and sql_select.cc
*/

int ha_example::delete_row(const uchar *buf)
{
  DBUG_ENTER("ha_example::delete_row");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  Positions an index cursor to the index specified in the handle. Fetches the
  row if available. If the key value is null, begin at the first key of the
  index.
*/

int ha_example::index_read_map(uchar *buf, const uchar *key,
                               key_part_map keypart_map MY_ATTRIBUTE((unused)),
                               enum ha_rkey_function find_flag
                               MY_ATTRIBUTE((unused)))
{
  int rc;
  DBUG_ENTER("ha_example::index_read");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read forward through the index.
*/

int ha_example::index_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_example::index_next");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read backwards through the index.
*/

int ha_example::index_prev(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_example::index_prev");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_first() asks for the first key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_example::index_first(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_example::index_first");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_last() asks for the last key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
*/
int ha_example::index_last(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_example::index_last");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  rnd_init() is called when the system wants the storage engine to do a table
  scan. See the example in the introduction at the top of this file to see when
  rnd_init() is called.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
*/
int ha_example::rnd_init(bool scan)
{
  DBUG_ENTER("ha_example::rnd_init");
  DBUG_RETURN(0);
}

int ha_example::rnd_end()
{
  DBUG_ENTER("ha_example::rnd_end");
  DBUG_RETURN(0);
}


/**
  @brief
  This is called for each row of the table scan. When you run out of records
  you should return HA_ERR_END_OF_FILE. Fill buff up with the row information.
  The Field structure for the table is the key to getting data into buf
  in a manner that will allow the server to understand it.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
*/
int ha_example::rnd_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_example::rnd_next");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str,
                       TRUE);
  rc= HA_ERR_END_OF_FILE;
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  position() is called after each call to rnd_next() if the data needs
  to be ordered. You can do something like the following to store
  the position:
  @code
  my_store_ptr(ref, ref_length, current_position);
  @endcode

  @details
  The server uses ref to store data. ref_length in the above case is
  the size needed to store current_position. ref is just a byte array
  that the server will maintain. If you are using offsets to mark rows, then
  current_position should be the offset. If it is a primary key like in
  BDB, then it needs to be a primary key.

  Called from filesort.cc, sql_select.cc, sql_delete.cc, and sql_update.cc.

  @see
  filesort.cc, sql_select.cc, sql_delete.cc and sql_update.cc
*/
void ha_example::position(const uchar *record)
{
  DBUG_ENTER("ha_example::position");
  DBUG_VOID_RETURN;
}


/**
  @brief
  This is like rnd_next, but you are given a position to use
  to determine the row. The position will be of the type that you stored in
  ref. You can use ha_get_ptr(pos,ref_length) to retrieve whatever key
  or position you saved when position() was called.

  @details
  Called from filesort.cc, records.cc, sql_insert.cc, sql_select.cc, and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_insert.cc, sql_select.cc and sql_update.cc
*/
int ha_example::rnd_pos(uchar *buf, uchar *pos)
{
  int rc;
  DBUG_ENTER("ha_example::rnd_pos");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str,
                       TRUE);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  ::info() is used to return information to the optimizer. See my_base.h for
  the complete description.

  @details
  Currently this table handler doesn't implement most of the fields really needed.
  SHOW also makes use of this data.

  You will probably want to have the following in your code:
  @code
  if (records < 2)
    records = 2;
  @endcode
  The reason is that the server will optimize for cases of only a single
  record. If, in a table scan, you don't know the number of records, it
  will probably be better to set records to two so you can return as many
  records as you need. Along with records, a few more variables you may wish
  to set are:
    records
    deleted
    data_file_length
    index_file_length
    delete_length
    check_time
  Take a look at the public variables in handler.h for more information.

  Called in filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc,
  sql_delete.cc, sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc,
  sql_table.cc, sql_union.cc, and sql_update.cc.

  @see
  filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc, sql_delete.cc,
  sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_table.cc,
  sql_union.cc and sql_update.cc
*/
int ha_example::info(uint flag)
{
  DBUG_ENTER("ha_example::info");
  DBUG_RETURN(0);
}


/**
  @brief
  extra() is called whenever the server wishes to send a hint to
  the storage engine. The myisam engine implements the most hints.
  ha_innodb.cc has the most exhaustive list of these hints.

    @see
  ha_innodb.cc
*/
int ha_example::extra(enum ha_extra_function operation)
{
  DBUG_ENTER("ha_example::extra");
  DBUG_RETURN(0);
}


/**
  @brief
  Used to delete all rows in a table, including cases of truncate and cases where
  the optimizer realizes that all rows will be removed as a result of an SQL statement.

  @details
  Called from item_sum.cc by Item_func_group_concat::clear(),
  Item_sum_count_distinct::clear(), and Item_func_group_concat::clear().
  Called from sql_delete.cc by mysql_delete().
  Called from sql_select.cc by JOIN::reinit().
  Called from sql_union.cc by st_select_lex_unit::exec().

  @see
  Item_func_group_concat::clear(), Item_sum_count_distinct::clear() and
  Item_func_group_concat::clear() in item_sum.cc;
  mysql_delete() in sql_delete.cc;
  JOIN::reinit() in sql_select.cc and
  st_select_lex_unit::exec() in sql_union.cc.
*/
int ha_example::delete_all_rows()
{
  DBUG_ENTER("ha_example::delete_all_rows");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  Used for handler specific truncate table.  The table is locked in
  exclusive mode and handler is responsible for reseting the auto-
  increment counter.

  @details
  Called from Truncate_statement::handler_truncate.
  Not used if the handlerton supports HTON_CAN_RECREATE, unless this
  engine can be used as a partition. In this case, it is invoked when
  a particular partition is to be truncated.

  @see
  Truncate_statement in sql_truncate.cc
  Remarks in handler::truncate.
*/
int ha_example::truncate()
{
  DBUG_ENTER("ha_example::truncate");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  This create a lock on the table. If you are implementing a storage engine
  that can handle transacations look at ha_berkely.cc to see how you will
  want to go about doing this. Otherwise you should consider calling flock()
  here. Hint: Read the section "locking functions for mysql" in lock.cc to understand
  this.

  @details
  Called from lock.cc by lock_external() and unlock_external(). Also called
  from sql_table.cc by copy_data_between_tables().

  @see
  lock.cc by lock_external() and unlock_external() in lock.cc;
  the section "locking functions for mysql" in lock.cc;
  copy_data_between_tables() in sql_table.cc.
*/
int ha_example::external_lock(THD *thd, int lock_type)
{
  DBUG_ENTER("ha_example::external_lock");
  DBUG_RETURN(0);
}


/**
  @brief
  The idea with handler::store_lock() is: The statement decides which locks
  should be needed for the table. For updates/deletes/inserts we get WRITE
  locks, for SELECT... we get read locks.

  @details
  Before adding the lock into the table lock handler (see thr_lock.c),
  mysqld calls store lock with the requested locks. Store lock can now
  modify a write lock to a read lock (or some other lock), ignore the
  lock (if we don't want to use MySQL table locks at all), or add locks
  for many tables (like we do when we are using a MERGE handler).

  Berkeley DB, for example, changes all WRITE locks to TL_WRITE_ALLOW_WRITE
  (which signals that we are doing WRITES, but are still allowing other
  readers and writers).

  When releasing locks, store_lock() is also called. In this case one
  usually doesn't have to do anything.

  In some exceptional cases MySQL may send a request for a TL_IGNORE;
  This means that we are requesting the same lock as last time and this
  should also be ignored. (This may happen when someone does a flush
  table when we have opened a part of the tables, in which case mysqld
  closes and reopens the tables and tries to get the same locks at last
  time). In the future we will probably try to remove this.

  Called from lock.cc by get_lock_data().

  @note
  In this method one should NEVER rely on table->in_use, it may, in fact,
  refer to a different thread! (this happens if get_lock_data() is called
  from mysql_lock_abort_for_thread() function)

  @see
  get_lock_data() in lock.cc
*/
THR_LOCK_DATA **ha_example::store_lock(THD *thd,
                                       THR_LOCK_DATA **to,
                                       enum thr_lock_type lock_type)
{
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK)
    lock.type=lock_type;
  *to++= &lock;
  return to;
}


/**
  @brief
  Used to delete a table. By the time delete_table() has been called all
  opened references to this table will have been closed (and your globally
  shared references released). The variable name will just be the name of
  the table. You will need to remove any files you have created at this point.

  @details
  If you do not implement this, the default delete_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from handler.cc by delete_table and ha_create_table(). Only used
  during create if the table_flag HA_DROP_BEFORE_CREATE was specified for
  the storage engine.

  @see
  delete_table and ha_create_table() in handler.cc
*/
int ha_example::delete_table(const char *name)
{
  DBUG_ENTER("ha_example::delete_table");
  /* This is not implemented but we want someone to be able that it works. */
  DBUG_RETURN(0);
}


/**
  @brief
  Renames a table from one name to another via an alter table call.

  @details
  If you do not implement this, the default rename_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from sql_table.cc by mysql_rename_table().

  @see
  mysql_rename_table() in sql_table.cc
*/
int ha_example::rename_table(const char * from, const char * to)
{
  DBUG_ENTER("ha_example::rename_table ");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  Given a starting key and an ending key, estimate the number of rows that
  will exist between the two keys.

  @details
  end_key may be empty, in which case determine if start_key matches any rows.

  Called from opt_range.cc by check_quick_keys().

  @see
  check_quick_keys() in opt_range.cc
*/
ha_rows ha_example::records_in_range(uint inx, key_range *min_key,
                                     key_range *max_key)
{
  DBUG_ENTER("ha_example::records_in_range");
  DBUG_RETURN(10);                         // low number to force index usage
}


/**
  @brief
  create() is called to create a database. The variable name will have the name
  of the table.

  @details
  When create() is called you do not need to worry about
  opening the table. Also, the .frm file will have already been
  created so adjusting create_info is not necessary. You can overwrite
  the .frm file at this point if you wish to change the table
  definition, but there are no methods currently provided for doing
  so.

  Called from handle.cc by ha_create_table().

  @see
  ha_create_table() in handle.cc
*/

int ha_example::create(const char *name, TABLE *table_arg,
                       HA_CREATE_INFO *create_info)
{
  DBUG_ENTER("ha_example::create");
  /*
    This is not implemented but we want someone to be able to see that it
    works.
  */
  DBUG_RETURN(0);
}


struct st_mysql_storage_engine example_storage_engine=
{ MYSQL_HANDLERTON_INTERFACE_VERSION };

static ulong srv_enum_var= 0;
static ulong srv_ulong_var= 0;
static double srv_double_var= 0;

const char *enum_var_names[]=
{
  "e1", "e2", NullS
};

TYPELIB enum_var_typelib=
{
  array_elements(enum_var_names) - 1, "enum_var_typelib",
  enum_var_names, NULL
};

static MYSQL_SYSVAR_ENUM(
  enum_var,                       // name
  srv_enum_var,                   // varname
  PLUGIN_VAR_RQCMDARG,            // opt
  "Sample ENUM system variable.", // comment
  NULL,                           // check
  NULL,                           // update
  0,                              // def
  &enum_var_typelib);             // typelib

static MYSQL_SYSVAR_ULONG(
  ulong_var,
  srv_ulong_var,
  PLUGIN_VAR_RQCMDARG,
  "0..1000",
  NULL,
  NULL,
  8,
  0,
  1000,
  0);

static MYSQL_SYSVAR_DOUBLE(
  double_var,
  srv_double_var,
  PLUGIN_VAR_RQCMDARG,
  "0.500000..1000.500000",
  NULL,
  NULL,
  8.5,
  0.5,
  1000.5,
  0);                             // reserved always 0

static MYSQL_THDVAR_DOUBLE(
  double_thdvar,
  PLUGIN_VAR_RQCMDARG,
  "0.500000..1000.500000",
  NULL,
  NULL,
  8.5,
  0.5,
  1000.5,
  0);

static struct st_mysql_sys_var* example_system_variables[]= {
  MYSQL_SYSVAR(enum_var),
  MYSQL_SYSVAR(ulong_var),
  MYSQL_SYSVAR(double_var),
  MYSQL_SYSVAR(double_thdvar),
  NULL
};

// this is an example of SHOW_FUNC and of my_snprintf() service
static int show_func_example(MYSQL_THD thd, struct st_mysql_show_var *var,
                             char *buf)
{
  var->type= SHOW_CHAR;
  var->value= buf; // it's of SHOW_VAR_FUNC_BUFF_SIZE bytes
  my_snprintf(buf, SHOW_VAR_FUNC_BUFF_SIZE,
              "enum_var is %lu, ulong_var is %lu, "
              "double_var is %f, %.6b", // %b is a MySQL extension
              srv_enum_var, srv_ulong_var, srv_double_var, "really");
  return 0;
}

struct example_vars_t
{
	ulong  var1;
	double var2;
	char   var3[64];
  bool   var4;
  bool   var5;
  ulong  var6;
};

example_vars_t example_vars= {100, 20.01, "three hundred", true, 0, 8250};

static st_mysql_show_var show_status_example[]=
{
  {"var1", (char *)&example_vars.var1, SHOW_LONG, SHOW_SCOPE_GLOBAL},
  {"var2", (char *)&example_vars.var2, SHOW_DOUBLE, SHOW_SCOPE_GLOBAL},
  {0,0,SHOW_UNDEF, SHOW_SCOPE_UNDEF} // null terminator required
};

static struct st_mysql_show_var show_array_example[]=
{
  {"array", (char *)show_status_example, SHOW_ARRAY, SHOW_SCOPE_GLOBAL},
  {"var3", (char *)&example_vars.var3, SHOW_CHAR, SHOW_SCOPE_GLOBAL},
  {"var4", (char *)&example_vars.var4, SHOW_BOOL, SHOW_SCOPE_GLOBAL},
  {0,0,SHOW_UNDEF, SHOW_SCOPE_UNDEF}
};

static struct st_mysql_show_var func_status[]=
{
  {"example_func_example", (char *)show_func_example, SHOW_FUNC, SHOW_SCOPE_GLOBAL},
  {"example_status_var5", (char *)&example_vars.var5, SHOW_BOOL, SHOW_SCOPE_GLOBAL},
  {"example_status_var6", (char *)&example_vars.var6, SHOW_LONG, SHOW_SCOPE_GLOBAL},
  {"example_status",  (char *)show_array_example, SHOW_ARRAY, SHOW_SCOPE_GLOBAL},
  {0,0,SHOW_UNDEF, SHOW_SCOPE_UNDEF}
};

mysql_declare_plugin(adbe)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &example_storage_engine,
  "ADBE",
  "DavisZhen",
  "ADBE storage engine",
  PLUGIN_LICENSE_GPL,
  example_init_func,                            /* Plugin Init */
  adbe_fini,                                         /* Plugin Deinit */
  0x0001 /* 0.1 */,
  func_status,                                  /* status variables */
  example_system_variables,                     /* system variables */
  NULL,                                         /* config options */
  0,                                            /* flags */
}
mysql_declare_plugin_end;
