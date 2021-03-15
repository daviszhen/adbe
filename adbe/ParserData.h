#ifndef __PARSER_DATA_H__
#define __PARSER_DATA_H__

#include <string>

class ParserData {
public:
	ParserData() {}
	virtual std::string toString()=0;
};

#endif // !__PARSER_DATA_H__
