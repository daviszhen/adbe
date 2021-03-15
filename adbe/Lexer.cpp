#include "Lexer.h"

void Lexer::nextToken() {
    while (beg < in.size()) {
        if (isspace(in[beg])) {
            ++beg;
        }else if (isdigit(in[beg])) {
            int tbeg = beg;
            while (beg < in.size() && isdigit(in[beg])) {
                ++beg;
            }
            tok = make_token(tbeg, beg - tbeg, in.substr(tbeg, beg - tbeg), TOK_INTEGER);
            break;
        }
        else if (isalpha(in[beg]) || in[beg] == '_') {
            int tbeg = beg;
            while (beg < in.size() && (isalpha(in[beg]) || in[beg] == '_')) {
                ++beg;
            }
            tok = make_token(tbeg, beg - tbeg, in.substr(tbeg, beg - tbeg), TOK_IDENTI);
            break;
        }
        else if (in[beg] == '\'') {//字符串常量 'abc'
            int tbeg = beg++;
            while (beg < in.size() && in[beg] != '\'') {//到下一个'停止
                ++beg;
            }
            if (beg >= in.size()) {
                tok = make_token(tbeg, beg - tbeg, in.substr(tbeg, beg - tbeg), TOK_INVALID);
            }
            else {
                ++beg;//跳过'
                tok = make_token(tbeg, beg - tbeg, in.substr(tbeg, beg - tbeg), TOK_STRING);
            }
            break;
        }
        else if (in[beg] == '.') {
            tok = make_token(beg++, 1, ".", TOK_DELIM);
            break;
        }
        else if (in[beg] == '=') {
            tok = make_token(beg++, 1, "=", TOK_DELIM);
            break;
        }
        else if (in[beg] == ',') {
            tok = make_token(beg++, 1, ",", TOK_DELIM);
            break;
        }
        else if (in[beg] == '(') {
            tok = make_token(beg++, 1, "(", TOK_DELIM);
            break;
        }
        else if (in[beg] == ')') {
            tok = make_token(beg++, 1, ")", TOK_DELIM);
            break;
        }
        else {
            sql_print_information("unrecoginzed tok character %c", in[beg]);
            exit(-1);
        }
    }
}

Token Lexer::make_token(int pos, int len, const std::string& raw, Tok_Type type) {
    Token tok{ pos,len,raw,type };
    if (type == TOK_INTEGER) {
        tok.ival = std::stoi(raw);
    }
    else if (type == TOK_IDENTI) {
        std::string lowcase = raw;
        for (auto& c : lowcase) {
            c = std::tolower(c);
        }
        if (keywords.count(lowcase)) {
            tok.type = TOK_KEYWORD;
        }
        tok.sval = lowcase;
    }
    else if (type == TOK_STRING) {
        tok.sval = raw.substr(1, len - 2);
    }
    return tok;
}

void Lexer::eatDelim(char d) {
    if (!matchDelim(d))
    {
        sql_print_information("want delim, but it's not now");
        exit(-1);
    }
    nextToken();
}

int Lexer::eatIntConstant() {
    if (!matchIntConstant())
    {
        sql_print_information("want int, but it's not now");
        exit(-1);
    }
    int i = tok.ival;
    nextToken();
    return i;
}

std::string Lexer::eatStringConstant() {
    if (!matchStringConstant())
    {
        sql_print_information("want string, but it's not now");
        exit(-1);
    }
    std::string s = tok.sval; //constants are not converted to lower case
    nextToken();
    return s;
}

void Lexer::eatKeyword(const std::string& w) {
    if (!matchKeyword(w))
    {
        sql_print_information("want keyword, but it's not now");
        exit(-1);
    }
    nextToken();
}

std::string Lexer::eatId() {
    if (!matchId())
    {
        sql_print_information("want id, but it's not now");
        exit(-1);
    }
    std::string s = tok.sval;
    nextToken();
    return s;
}