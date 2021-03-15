#include "Parser.h"
#include "QueryData.h"
#include "ModifyData.h"

std::string Parser::field() {
    return lex->eatId();
}

Constant* Parser::constant() {
    if (lex->matchStringConstant())
        return new Constant(lex->eatStringConstant());
    else
        return new Constant(lex->eatIntConstant());
}

Expression* Parser::expression() {
    if (lex->matchId())
        return new Expression(field());
    else
        return new Expression(constant());
}

Term* Parser::term() {
    Expression* lhs = expression();
    lex->eatDelim('=');
    Expression* rhs = expression();
    return new Term(lhs, rhs);
}

Predicate* Parser::predicate() {
    Predicate* pred = new Predicate(term());
    if (lex->matchKeyword("and")) {
        lex->eatKeyword("and");
        pred->conjoinWith(*predicate());
    }
    return pred;
}

QueryData* Parser::query() {
    lex->eatKeyword("select");
    std::list<std::string> fields = selectList();
    lex->eatKeyword("from");
    std::list<std::string> tables = tableList();
    Predicate* pred = new Predicate();
    if (lex->matchKeyword("where")) {
        lex->eatKeyword("where");
        pred = predicate();
    }
    return new QueryData(fields, tables, pred);
}

std::list<std::string> Parser::selectList() {
    std::list<std::string> L;
    L.push_back(field());
    if (lex->matchDelim(',')) {
        lex->eatDelim(',');
        std::list<std::string> rest = selectList();
        L.insert(L.end(),rest.begin(),rest.end());
    }
    return L;
}

std::list<std::string> Parser::tableList() {
    std::list<std::string> L;
    L.push_back(lex->eatId());
    if (lex->matchDelim(',')) {
        lex->eatDelim(',');
        std::list<std::string> rest = tableList();
        L.insert(L.end(),rest.begin(),rest.end());
    }
    return L;
}

ParserData* Parser::updateCmd() {
    if (lex->matchKeyword("insert"))
        return insert();
    else if (lex->matchKeyword("delete"))
        return remove();
    else if (lex->matchKeyword("update"))
        return modify();
    else
        return create();
}

ParserData* Parser::create() {
    lex->eatKeyword("create");
    if (lex->matchKeyword("table"))
        return createTable();
    else if (lex->matchKeyword("view"))
        return createView();
    else
        return createIndex();
}

DeleteData* Parser::remove() {
    lex->eatKeyword("delete");
    lex->eatKeyword("from");
    std::string tblname = lex->eatId();
    Predicate* pred = new Predicate();
    if (lex->matchKeyword("where")) {
        lex->eatKeyword("where");
        pred = predicate();
    }
    return new DeleteData(tblname, pred);
}

InsertData* Parser::insert() {
    lex->eatKeyword("insert");
    lex->eatKeyword("into");
    std::string tblname = lex->eatId();
    lex->eatDelim('(');
    std::list<std::string> flds = fieldList();
    lex->eatDelim(')');
    lex->eatKeyword("values");
    lex->eatDelim('(');
    std::list<Constant> vals = constList();
    lex->eatDelim(')');
    return new InsertData(tblname, flds, vals);
}

std::list<std::string> Parser::fieldList() {
    std::list<std::string> L;
    L.push_back(field());
    if (lex->matchDelim(',')) {
        lex->eatDelim(',');
        std::list<std::string> rest = fieldList();
        L.insert(L.end(),rest.begin(),rest.end());
    }
    return L;
}

std::list<Constant> Parser::constList() {
    std::list<Constant> L;
    L.push_back(*constant());
    if (lex->matchDelim(',')) {
        lex->eatDelim(',');
        std::list<Constant> rest = constList();
        L.insert(L.end(),rest.begin(),rest.end());
    }
    return L;
}

ModifyData* Parser::modify() {
    lex->eatKeyword("update");
    std::string tblname = lex->eatId();
    lex->eatKeyword("set");
    std::string fldname = field();
    lex->eatDelim('=');
    Expression* newval = expression();
    Predicate* pred = new Predicate();
    if (lex->matchKeyword("where")) {
        lex->eatKeyword("where");
        pred = predicate();
    }
    return new ModifyData(tblname, fldname, newval, pred);
}

CreateTableData* Parser::createTable() {
    lex->eatKeyword("table");
    std::string tblname = lex->eatId();
    lex->eatDelim('(');
    Schema sch = fieldDefs();
    lex->eatDelim(')');
    return new CreateTableData(tblname, sch);
}

Schema Parser::fieldDefs() {
    Schema schema = fieldDef();
    if (lex->matchDelim(',')) {
        lex->eatDelim(',');
        Schema schema2 = fieldDefs();
        schema.addAll(schema2);
    }
    return schema;
}

Schema Parser::fieldDef() {
    std::string fldname = field();
    return fieldType(fldname);
}

Schema Parser::fieldType(const std::string& fldname) {
    Schema schema;
    if (lex->matchKeyword("int")) {
        lex->eatKeyword("int");
        schema.addIntField(fldname);
    }
    else {
        lex->eatKeyword("varchar");
        lex->eatDelim('(');
        int strLen = lex->eatIntConstant();
        lex->eatDelim(')');
        schema.addStringField(fldname, strLen);
    }
    return schema;
}

CreateViewData* Parser::createView() {
    lex->eatKeyword("view");
    std::string viewname = lex->eatId();
    lex->eatKeyword("as");
    QueryData* qd = query();
    return new CreateViewData(viewname, qd);
}

CreateIndexData* Parser::createIndex() {
    lex->eatKeyword("index");
    std::string idxname = lex->eatId();
    lex->eatKeyword("on");
    std::string tblname = lex->eatId();
    lex->eatDelim('(');
    std::string fldname = field();
    lex->eatDelim(')');
    return new CreateIndexData(idxname, tblname, fldname);
}