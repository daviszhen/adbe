#ifndef __LEXER_H__
#define __LEXER_H__

#include <mysql/thread_pool_priv.h>
#include <string>
#include <unordered_set>
#include <vector>

enum Tok_Type {
    TOK_INVALID,
    TOK_IDENTI,
    TOK_KEYWORD,
    TOK_STRING,//'abc'
    TOK_INTEGER,//10
    TOK_DELIM,//. = , ( ) 
    TOK_DOT,//.
    TOK_EQUAL,//=
    TOK_COMMA,//,
    TOK_EOF,//end
};

struct Token {
    int pos;
    int len;
    std::string raw_str;//Ô­Ê¼´®
    Tok_Type type;

    int ival;
    std::string sval;
};

class Lexer {
public:
    
public:
    Lexer(const std::string& instr){
         keywords= {
            "select", "from", "where", "and",
            "insert", "into", "values", "delete", "update", "set",
            "create", "table", "int", "varchar", "view", "as", "index", "on"
        };
        this->in = instr;
        tok.type = TOK_INVALID;
        this->beg = 0;
        nextToken();
    }

    bool matchDelim(char d) {
        return tok.type == TOK_DELIM && tok.raw_str == std::string(1,d);
    }

    bool matchIntConstant() {
        return tok.type == TOK_INTEGER;
    }

    bool matchStringConstant() {
        return tok.type == TOK_STRING;
    }

    bool matchKeyword(const std::string& w) {
        return tok.type == TOK_KEYWORD && tok.sval == w;
    }

    bool matchId() {
        return  tok.type == TOK_IDENTI;
    }

    void eatDelim(char d);

    int eatIntConstant();

    std::string eatStringConstant();

    void eatKeyword(const std::string& w);

    std::string eatId();
private:
    void nextToken();

    Token make_token(int pos, int len, const std::string& raw, Tok_Type type);

private:
    Token tok;
    int beg;
    std::unordered_set<std::string> keywords;
    std::string in;
};

#endif // !__LEXER_H__
