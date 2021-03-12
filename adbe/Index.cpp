#include "Index.h"

HashIndex::HashIndex(Transaction* tx, const std::string& idxname, Layout* layout){
	this->tx = tx;
	this->idxname = idxname;
	this->layout = layout;
}

void HashIndex::beforeFirst(Constant& searchkey) {
	close();
	this->searchkey = searchkey;
	int bucket = searchkey.hashCode() % NUM_BUCKETS;
	std::string tblname = idxname + std::to_string(bucket);//?
	ts = new TableScan(tx, tblname, layout);
}

bool HashIndex::next() {
	while (ts->next())
		if (ts->getVal("dataval") == searchkey)
			return true;
	return false;
}

RID HashIndex::getDataRid() {
	int blknum = ts->getInt("block");
	int id = ts->getInt("id");
	return RID(blknum, id);
}

void HashIndex::insert(Constant& val, RID& rid) {
	beforeFirst(val);
	ts->insert();
	ts->setInt("block", rid.blockNumber());
	ts->setInt("id", rid.slot());
	ts->setVal("dataval", val);
}

void HashIndex::remove(Constant& val, RID& rid) {
	beforeFirst(val);
	while (next())
		if (getDataRid() == rid) {
			ts->remove();
			return;
		}
}

void HashIndex::close() {
	if (ts != nullptr) {
		ts->close();
		delete ts;
	}
}

int HashIndex::searchCost(int numblocks, int rpb) {
	return numblocks / HashIndex::NUM_BUCKETS;
}