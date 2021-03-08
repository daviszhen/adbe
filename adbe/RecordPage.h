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

	//����ֵ -1 ��ʾ û�гɹ�
	//����ֵ -2 ��ʾ ����ʧ��
	int insertAfter(int slot);

	BlockId* block();

private:
	bool setFlag(int slot, int flag);

	//����ֵ -1 ��ʾ û���ҵ�
	//����ֵ -2 ��ʾ ����ʧ��
	int searchAfter(int slot, int flag);

	bool isValidSlot(int slot);

	int offset(int slot);
private:
	Transaction* tx;
	BlockId* blk;
	Layout* layout;
};
#endif // !__RECORD_PAGE_H__
