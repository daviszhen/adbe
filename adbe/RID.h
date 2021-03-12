#ifndef __RID_H__
#define __RID_H__

#include <string>

//¼ÇÂ¼±êÊ¶
class RID {
public:
	RID(int blknum, int slot) {
		this->blknum = blknum;
		this->slt = slot;
	}

	int blockNumber() {
		return blknum;
	}

	int slot() {
		return slt;
	}

	bool operator==(const RID& o) {
		return blknum == o.blknum && slt == o.slt;
	}

	std::string toString() {
		return "[" + std::to_string(blknum) + ", " + std::to_string(slt) + "]";
	}
private:
	int blknum;
	int slt;
};
#endif // !__RID_H__
