#ifndef __INDEX_H__
#define __INDEX_H__

#include "Scan.h"
#include "Transaction.h"

class Index {
public:
	void beforeFirst(Constant& searchkey);

	bool next();

	RID  getDataRid();

	void insert(Constant& dataval, RID& datarid);

	void remove(Constant& dataval, RID& datarid);

	void close();
};

class HashIndex : public Index {
public:
	HashIndex(Transaction* tx, const std::string& idxname, Layout* layout);

	void beforeFirst(Constant& searchkey);

	bool next();

	RID getDataRid();

	void insert(Constant& val, RID& rid);

	void remove(Constant& val, RID& rid);

	void close();

	static int searchCost(int numblocks, int rpb);
public:
	const static int NUM_BUCKETS = 100;
private:
	Transaction* tx;
	std::string idxname;
	Layout* layout;
	Constant searchkey;
	TableScan* ts;
};
#endif // !__INDEX_H__
