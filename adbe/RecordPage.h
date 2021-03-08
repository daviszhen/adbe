#ifndef __RECORD_PAGE_H__
#define __RECORD_PAGE_H__

#include "Transaction.h"
#include "blockid.h"
#include "Layout.h"



class RecordPage {
public:
	const static int EMPTY = 0;
	const static int USED = 1;
	
	RecordPage(Transaction* tx, BlockId* blk, Layout* layout);

	bool getInt(int slot, const std::string& fldname, int& out);

	bool getString(int slot, const std::string& fldname, std::string& out);

	bool setInt(int slot, const std::string& fldname, int val);

	bool setString(int slot, const std::string& fldname, const std::string& val);

	bool remove(int slot);

	bool format();

	int nextAfter(int slot);

	//返回值 -1 表示 没有成功
	//返回值 -2 表示 事务失败
	int insertAfter(int slot);

	BlockId* block();

private:
	bool setFlag(int slot, int flag);

	//返回值 -1 表示 没有找到
	//返回值 -2 表示 事务失败
	int searchAfter(int slot, int flag);

	bool isValidSlot(int slot);

	int offset(int slot);
private:
	Transaction* tx;
	BlockId* blk;
	Layout* layout;
};
#endif // !__RECORD_PAGE_H__
