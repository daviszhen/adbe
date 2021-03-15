#ifndef __PARSER_H__
#define __PARSER_H__

#include "Lexer.h"
#include "Scan.h"
#include "Term.h"
#include "Predicate.h"
#include "QueryData.h"
#include "CreateIndexData.h"
#include "CreateViewData.h"
#include "CreateTableData.h"
#include "ModifyData.h"
#include "InsertData.h"
#include "DeleteData.h"

#define	PDT_INVALID  -1
#define	PDT_CREATE_INDEX_DATA 0
#define	PDT_CREATE_TABLE_DATA 1
#define	PDT_CREATE_VIEW_DATA 2
#define	PDT_DELETE_DATA 3
#define	PDT_INSERT_DATA 4
#define	PDT_MODIFY_DATA 5
#define	PDT_QUERY_DATA 6

class Parser {
public:

	Parser(const std::string& in) {
		lex = new Lexer(in);
	}
	~Parser() {
		delete lex;
	}

	std::string field();

	Constant* constant();

	Expression* expression();

	Term* term();

	Predicate* predicate();

	QueryData* query();

	std::list<std::string> selectList();

	std::list<std::string> tableList();
	
	ParserData* updateCmd();

	ParserData* create();

	DeleteData* remove();

	InsertData* insert();

	std::list<std::string> fieldList();

	std::list<Constant> constList();

	ModifyData* modify();

	CreateTableData* createTable();

	Schema fieldDefs();

	Schema fieldDef();

	Schema fieldType(const std::string& fldname);

	CreateViewData* createView();
	
	CreateIndexData* createIndex();
private:
	Lexer* lex;
};

#endif