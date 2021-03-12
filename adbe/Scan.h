#ifndef __SCAN_H__
#define __SCAN_H__

#include <string>

#include "RID.h"
#include "Transaction.h"
#include "Layout.h"
#include "RecordPage.h"

class Constant {
public:
	Constant() {
		isInt = false;
		ival = 0;
		sval = "";
		isValid = false;
	}
	Constant(int ival) {
		this->ival = ival;
		this->isInt = true;
		isValid = true;
	}

	Constant(const std::string& sval) {
		this->sval = sval;
		this->isInt = false;
		isValid = true;
	}

	bool valid()const {
		return isValid;
	}

	int asInt() {
		return ival;
	}

	std::string asString() {
		return sval;
	}

	bool operator==(const Constant& o)const {
		if (isInt) {
			return ival == o.ival;
		}
		else {
			return sval == o.sval;
		}
	}

	int hashCode() {
		if (isInt) {
			std::hash<int> ihash;
			return ihash(ival);
		}
		else {
			std::hash<std::string> shash;
			return shash(sval);
		}
	}

	std::string toString() {
		return isInt ? std::to_string(ival) : sval;
	}
private:
	int ival;
	std::string  sval;
	bool isInt;
	bool isValid;
};

class Scan;

//���ʽ���ֶ�������
class Expression {
public:
	Expression(Constant* val) {
		this->val = val;
	}

	Expression(const std::string& fldname) {
		this->val = nullptr;
		this->fldname = fldname;
	}

	~Expression() {
		delete val;
	}

	//��Scan�ĵ�ǰ��¼�ϣ�������ʽ��ֵ
	//�ֶ��� ���� �ֶ�ֵ
	//����   ���� ����ֵ
	Constant evaluate(Scan& s);

	//�ֶ���
	bool isFieldName() {
		return val == nullptr;
	}

	//����ֵ
	Constant asConstant() {
		return *val;
	}

	std::string asFieldName() {
		return fldname;
	}

	//�ֶ��� �Ƿ���Schema��
	//����ֵ���κ�Schema��
	bool appliesTo(Schema& sch) {
		return (val != nullptr) ? true : sch.hasField(fldname);
	}

	std::string toString() {
		return (val != nullptr) ? val->toString() : fldname;
	}
private:
	Constant* val;
	std::string fldname;
};

//��ʾ��ϵ����query
class Scan {
public:
	//��λscan����һ����¼֮ǰ.
	virtual void beforeFirst()=0;

	//�ƶ�scan����һ����¼
	virtual bool next() = 0;

	//��ǰ��¼�����ֶ�ֵ
	virtual int getInt(const std::string& fldname)=0;

	//��ǰ��¼�ַ����ֶ�ֵ
	virtual std::string getString(const std::string& fldname) = 0;

	//��ǰ��¼�ֶ�ֵ
	virtual Constant getVal(const std::string& fldname) = 0;

	//scan�и��ֶ���?
	virtual bool hasField(const std::string& fldname) = 0;

	virtual void close() = 0;
};

class UpdateScan : public Scan {
public:
	//���ü�¼���ֶ�ֵ
	virtual void setVal(const std::string& fldname, Constant& val) {}

	//
	virtual void setInt(const std::string& fldname, int val) {}

	virtual void setString(const std::string& fldname, const std::string& val) {}

	//��scan�в���һ���¼�¼
	virtual void insert(){}

	//��scan��ɾ���¼�¼
	virtual void remove(){}

	//���ص�ǰ��¼��id
	virtual RID  getRid() { return RID(-1,-1); }

	//��scan��λ��rid
	virtual void moveToRid(const RID& rid){}
};

//��ʾ������¼����
class TableScan : public UpdateScan{
public:
	TableScan(Transaction* tx, const std::string& tblname, Layout* layout);
	~TableScan();
	void beforeFirst();

	bool next();

	int getInt(const std::string& fldname);

	std::string getString(const std::string& fldname);

	Constant getVal(const std::string& fldname);

	bool hasField(const std::string& fldname);

	void close();

	void setInt(const std::string& fldname, int val);

	void setString(const std::string& fldname, const std::string& val);

	void setVal(const std::string& fldname, Constant& val);

	void insert();

	void remove();

	void moveToRid(RID& rid);

	RID getRid();

private:
	void moveToBlock(int blknum);

	void moveToNewBlock();

	bool atLastBlock();

	void switchRecordPage(RecordPage* rp);
private:
	Transaction* tx;
	Layout* layout;
	RecordPage* rp;
	std::string filename;
	int currentslot;
};

class ProductScan : public Scan {
public:
	ProductScan(Scan* s1, Scan* s2) {
		this->s1 = s1;
		this->s2 = s2;
		beforeFirst();
	}
	
	//LHS��λ����һ����¼��
	//RHS��λ����һ����¼֮ǰ.
	void beforeFirst() {
		s1->beforeFirst();
		s1->next();
		s2->beforeFirst();
	}

	//nested loop �㷨
	//������ܣ����ƶ���RHS����һ����¼��
	//�������ƶ���LHS����һ����¼�����ƶ���RHS�ĵ�һ����¼
	bool next() {
		if (s2->next())
			return true;
		else {
			s2->beforeFirst();
			return s2->next() && s1->next();
		}
	}

	int getInt(const std::string& fldname) {
		if (s1->hasField(fldname))
			return s1->getInt(fldname);
		else
			return s2->getInt(fldname);
	}

	std::string getString(const std::string& fldname) {
		if (s1->hasField(fldname))
			return s1->getString(fldname);
		else
			return s2->getString(fldname);
	}

	Constant getVal(const std::string& fldname) {
		if (s1->hasField(fldname))
			return s1->getVal(fldname);
		else
			return s2->getVal(fldname);
	}

	bool hasField(const std::string& fldname) {
		return s1->hasField(fldname) || s2->hasField(fldname);
	}

	void close() {
		s1->close();
		s2->close();
	}
private:
	Scan* s1, * s2;
};

class ProjectScan: public Scan {
public:
	ProjectScan(Scan* s, const std::list<std::string>& fieldlist) {
		this->s = s;
		this->fieldlist = fieldlist;
	}

	void beforeFirst() {
		s->beforeFirst();
	}

	bool next() {
		return s->next();
	}

	int getInt(const std::string& fldname) {
		if (hasField(fldname))
			return s->getInt(fldname);
		else
			assert(0);
	}

	std::string getString(const std::string& fldname) {
		if (hasField(fldname))
			return s->getString(fldname);
		else
			assert(0);
	}

	Constant getVal(const std::string& fldname) {
		if (hasField(fldname))
			return s->getVal(fldname);
		else
			assert(0);
	}

	bool hasField(const std::string& fldname) {
		return std::find(fieldlist.begin(),fieldlist.end(),fldname) !=fieldlist.end();
	}

	void close() {
		s->close();
	}
private:
	Scan* s;
	std::list<std::string> fieldlist;
};
#endif // !__SCAN_H__
