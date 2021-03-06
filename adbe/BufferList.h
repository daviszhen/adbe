#ifndef __BUFFER_LIST_H__
#define __BUFFER_LIST_H__

#include "BufferMgr.h"

#include <unordered_map>
#include <list>
#include <algorithm>

class BufferList {
public:
	BufferList(BufferMgr* bm) {
		this->bm = bm;
	}

	Buffer* getBuffer(BlockId* blk) {
		return buffers[blk];
	}

	//没有得到缓冲区，返回false
	bool pin(BlockId* blk) {
		Buffer* buff = bm->pin(blk);
		if (buff == nullptr) {
			return false;
		}
		buffers[blk]= buff;
		pins.push_back(blk);
		return true;
	}

	void unpin(BlockId* blk) {
		Buffer* buff = buffers[blk];
		bm->unpin(buff);
		pins.remove(blk);
		if (std::find(pins.begin(),pins.end(),blk) == pins.end())
			buffers.erase(blk);
	}

	void unpinAll() {
		for (BlockId* blk : pins) {
			Buffer* buff = buffers[blk];
			bm->unpin(buff);
		}
		buffers.clear();
		pins.clear();
	}
private:
	std::unordered_map<BlockId*, Buffer*> buffers;
	std::list<BlockId*> pins;
	BufferMgr* bm;
};
#endif // !__BUFFER_LIST_H__
