#ifndef __BLOCK_ID_H__
#define __BLOCK_ID_H__

#include <string>

class BlockId {
public:
	BlockId(const std::string _filename, int _blknum) {
		filename = _filename;
		blknum = _blknum;
	}

	static BlockId* New(const std::string _filename, int _blknum) {
		return new BlockId(_filename, _blknum);
	}

	bool operator==(const BlockId& ob)const {
		return filename == ob.filename && blknum == ob.blknum;
	}

	const std::string fileName() const{
		return filename;
	}

	int number() const {
		return blknum;
	}

	std::string to_str()const {
		return filename + "@"+ std::to_string(blknum);
	}
private:
	std::string filename;//ÎÄ¼þÃû³Æ
	int blknum;//Âß¼­¿éºÅ
};

#endif