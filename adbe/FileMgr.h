#ifndef __FILE_MGR_H__
#define __FILE_MGR_H__

#include <fstream>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>

#include "sql_class.h"           // MYSQL_HANDLERTON_INTERFACE_VERSION
#include "ha_example.h"
#include "probes_mysql.h"
#include "sql_plugin.h"

#include "blockid.h"
#include "page.h"
#include "bytebuffer.h"

class FileMgr {
public:
	FileMgr(int blocksize){
		this->blocksize = blocksize;
		this->zeroBb = ByteBuffer::New(blocksize);
		mysql_mutex_init(NULL, &fileMgrMutex, MY_MUTEX_INIT_FAST);
	}

	~FileMgr() {
		delete zeroBb;
		for (auto pr : openFiles) {
			if (pr.second) {
				pr.second->flush();
				pr.second->close();
				delete pr.second;
			}
		}
		mysql_mutex_destroy(&fileMgrMutex);
	}

	static FileMgr* New(int bsize) {
		return new FileMgr(bsize);
	}
	void read(const BlockId& blk, Page& p) {
		mysql_mutex_lock(&fileMgrMutex);
		std::fstream* f = getFile(blk.fileName());
		extendFileIfNeeded(f, blk.number());
		std::istream& in1 = f->seekg(blk.number() * blocksize);
		iStreamErrorHandle(f, in1);
		ByteBuffer* bb = p.contents();
		std::istream& in2 = f->read(bb->getBuffer(),blocksize);
		iStreamErrorHandle(f, in2);
		mysql_mutex_unlock(&fileMgrMutex);
	}

	void write(const BlockId& blk, const Page& p) {
		mysql_mutex_lock(&fileMgrMutex);
		std::fstream* f = getFile(blk.fileName());
		std::ostream& o1 = f->seekp(blk.number() * blocksize);
		oStreamErrorHandle(f, o1);
		const ByteBuffer* bb = p.contents();
		std::ostream& o2 = f->write(bb->getBuffer(), blocksize);
		oStreamErrorHandle(f, o2);
		mysql_mutex_unlock(&fileMgrMutex);
	}

	BlockId* append(const std::string& filename) {
		mysql_mutex_lock(&fileMgrMutex);
		int newblknum = length_unsafe(filename);
		BlockId* blk = BlockId::New(filename, newblknum);
		std::fstream* f = getFile(blk->fileName());
		std::ostream& o1 = f->seekp(blk->number() * blocksize);
		oStreamErrorHandle(f, o1);
		std::ostream& o2 = f->write(zeroBb->getBuffer(), blocksize);//垃圾值
		oStreamErrorHandle(f, o2);
		mysql_mutex_unlock(&fileMgrMutex);
		return blk;
	}

	//块长度
	int length(const std::string& filename) {
		mysql_mutex_lock(&fileMgrMutex);
		std::fstream* f = getFile(filename);
		int length = fileLength(f);
		mysql_mutex_unlock(&fileMgrMutex);
		return length / blocksize;
	}

	int blockSize() {
		return blocksize;
	}

private:
	//块长度
	int length_unsafe(const std::string& filename) {
		std::fstream* f = getFile(filename);
		int length = fileLength(f);
		return length / blocksize;
	}

	//块长度
	int length_unsafe(std::fstream* f) {
		int length = fileLength(f);
		return length / blocksize;
	}

	//字节长度
	int fileLength(std::fstream* f) {
		// get length of file
		std::istream& in1 = f->seekg(0, f->end);
		iStreamErrorHandle(f, in1);
		std::streampos p = f->tellg();
		assert(p != -1);
		std::istream& in2 = f->seekg(0, f->beg);
		iStreamErrorHandle(f, in2);
		return static_cast<int>(p);
	}

	//根据blockid的位置扩展文件
	void extendFileIfNeeded(std::fstream* f, int blockid) {
		int blockLenth = length_unsafe(f);
		if (blockid >= blockLenth) {
			for (int bid = blockLenth; bid <= blockid; ++bid) {
				std::ostream& o1 = f->seekp(bid * blocksize, f->beg);
				oStreamErrorHandle(f, o1);
				std::ostream& o2 = f->write(zeroBb->getBuffer(), blocksize);//垃圾值
				oStreamErrorHandle(f, o2);
			}
		}
		blockLenth = length_unsafe(f);
		assert(blockid < blockLenth);
	}

	//输入文件流错误处理
	//is 是 f的某个操作返回的输入文件流
	void iStreamErrorHandle(const std::fstream* f,const std::istream& is) {
		if (is.good()) {
			//nothing
		}
		else if (is.fail()) {
			std::cerr << "istream operation failed "
				<< "good() " << is.good() << ", fail() " << is.fail() << ", bad() " << is.bad() << ", eof() " << is.eof() << std::endl;
			assert(0);
		}
		else if (is.eof()) {
			std::cerr << "istream end " 
			<< "good() " << is.good() << ", fail() " << is.fail() << ", bad() " << is.bad() << ", eof() " << is.eof() << std::endl;
			assert(0);
		}
		else if (is.bad()) {
			std::cerr << "istream error "
				<< "good() " << is.good() << ", fail() " << is.fail() << ", bad() " << is.bad() << ", eof() " << is.eof() << std::endl;
			assert(0);
		}
	}

	//输出文件流错误处理
	//os 是 f的某个操作返回的输出文件流
	void oStreamErrorHandle(const std::fstream* f, const std::ostream& s) {
		if (s.good()) {
			//nothing
		}
		else if (s.fail()) {
			std::cerr << "ostream operation failed "
				<< "good() " << s.good() << ", fail() " << s.fail() << ", bad() " << s.bad() << ", eof() " << s.eof() << std::endl;
			assert(0);
		}
		else if (s.eof()) {
			std::cerr << "ostream end "
				<< "good() " << s.good() << ", fail() " << s.fail() << ", bad() " << s.bad() << ", eof() " << s.eof() << std::endl;
			assert(0);
		}
		else if (s.bad()) {
			std::cerr << "ostream damage "
				<< "good() " << s.good() << ", fail() " << s.fail() << ", bad() " << s.bad() << ", eof() " << s.eof() << std::endl;
			assert(0);
		}
	}

	std::fstream* getFile(const std::string& filename) {
		std::fstream* f = nullptr;
		if (openFiles.count(filename)) {
			f = openFiles[filename];
		}
		else {
			f = new std::fstream(filename, std::fstream::in | std::fstream::out | std::fstream::binary);
			assert(f != nullptr);
			if (!f->is_open()) {//打开文件失败，文件可能不存在
				//先关闭
				f->close();
				delete f;

				//先创建文件.
				f = new std::fstream(filename, std::fstream::out | std::fstream::binary);
				assert(f != nullptr);
				assert(f->is_open());
				//再关闭
				f->close();

				//再带上可读标记打开文件
				f->open(filename, std::fstream::in | std::fstream::out | std::fstream::binary);
				assert(f->is_open());
			}
			openFiles[filename] = f;
		}
		return f;
	}
private:
	mysql_mutex_t fileMgrMutex;
	int blocksize;
	ByteBuffer* zeroBb;
	std::unordered_map<std::string, std::fstream*> openFiles;
};

#endif // !__FILE_MGR_H__