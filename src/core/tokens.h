#ifndef TOKENS_H
#define TOKENS_H

#include <string>
#include <vector>

enum TokenID { 
    TK_ID = 1, 
    TK_NUMBER, 
    TK_PLUS, 
    TK_MINUS, 
    TK_STAR, 
    TK_SLASH, 
    TK_LPAREN, 
    TK_RPAREN, 
    TK_WS 
};

extern std::vector<std::string> tokenNames;

struct Token { 
    int id; 
    std::string lexeme; 
    int pos; 
};

// ADD THIS NEW FILE for the implementation:
#endif // TOKENS_H