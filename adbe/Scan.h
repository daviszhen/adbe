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

//表达式。字段名或常数
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

	//在Scan的当前记录上，计算表达式的值
	//字段名 生成 字段值
	//常数   生成 常数值
	Constant evaluate(Scan& s);

	//字段名
	bool isFieldName() {
		return val == nullptr;
	}

	//常数值
	Constant asConstant() {
		return *val;
	}

	std::string asFieldName() {
		return fldname;
	}

	//字段名 是否在Schema中
	//常数值在任何Schema中
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

//表示关系代数query
class Scan {
public:
	//定位scan到第一个记录之前.
	virtual void beforeFirst()=0;

	//移动scan到下一个记录
	virtual bool next() = 0;

	//当前记录整数字段值
	virtual int getInt(const std::string& fldname)=0;

	//当前记录字符串字段值
	virtual std::string getString(const std::string& fldname) = 0;

	//当前记录字段值
	virtual Constant getVal(const std::string& fldname) = 0;

	//scan有该字段吗?
	virtual bool hasField(const std::string& fldname) = 0;

	virtual void close() = 0;
};

class UpdateScan : public Scan {
public:
	//设置记录的字段值
	virtual void setVal(const std::string& fldname, Constant& val) {}

	//
	virtual void setInt(const std::string& fldname, int val) {}

	virtual void setString(const std::string& fldname, const std::string& val) {}

	//在scan中插入一个新记录
	virtual void insert(){}

	//从scan中删除新记录
	virtual void remove(){}

	//返回当前记录的id
	virtual RID  getRid() { return RID(-1,-1); }

	//将scan定位到rid
	virtual void moveToRid(const RID& rid){}
};

//表示任意大记录数组
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
	
	//LHS定位到第一条记录。
	//RHS定位到第一条记录之前.
	void beforeFirst() {
		s1->beforeFirst();
		s1->next();
		s2->beforeFirst();
	}

	//nested loop 算法
	//如果可能，先移动到RHS的下一个记录。
	//否则，先移动到LHS的下一个记录，再移动到RHS的第一个记录
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
