#ifndef __SELECT_SCAN_H__
#define __SELECT_SCAN_H__

#include "Scan.h"
#include "Predicate.h"

class SelectScan : public UpdateScan {
public:
	SelectScan(Scan* s, Predicate* pred) {
		this->s = s;
		this->pred = pred;
	}

	void beforeFirst() {
		s->beforeFirst();
	}

	bool next() {
		while (s->next()) {
			if (pred->isSatisfied(*s))
				return true;
		}
		return false;
	}

	int getInt(const std::string& fldname) {
		return s->getInt(fldname);
	}

	std::string getString(const std::string& fldname) {
		return s->getString(fldname);
	}

	Constant getVal(const std::string& fldname) {
		return s->getVal(fldname);
	}

	bool hasField(const std::string& fldname) {
		return s->hasField(fldname);
	}

	void close() {
		s->close();
	}

	void setInt(const std::string& fldname, int val) {
		UpdateScan* us = convertTo();
		us->setInt(fldname, val);
	}

	void setString(const std::string& fldname, const std::string& val) {
		UpdateScan* us = convertTo();
		us->setString(fldname, val);
	}

	void setVal(const std::string& fldname, Constant& val) {
		UpdateScan* us = convertTo();
		us->setVal(fldname, val);
	}

	void remove() {
		UpdateScan* us = convertTo();
		us->remove();
	}

	void insert() {
		UpdateScan* us = convertTo();
		us->insert();
	}

	RID getRid() {
		UpdateScan* us = convertTo();
		return us->getRid();
	}

	void moveToRid(RID& rid) {
		UpdateScan* us = convertTo();
		us->moveToRid(rid);
	}

private:
	UpdateScan* convertTo() {
		UpdateScan* us = nullptr;
		if ((us = dynamic_cast<UpdateScan*>(s)) != nullptr) {
			return us;
		}
		else {
			assert(0);
		}
	}
private:
	Scan *s;
	Predicate* pred;
};
#endif // !__SELECT_SCAN_H__
