#ifndef PARSER_H
#define PARSER_H

#include "core/tokens.h"
#include "grammar.h"
#include <vector>
#include <string>

class Parser {
public:
    Parser();
    
    bool parseAll(const std::vector<Token> &tokens);
    bool stepParse(const std::vector<Token> &tokens);
    void reset();
    
    std::vector<std::string> getStack() const { return stack; }
    bool isDone() const { return done; }
    int getCurrentPosition() const { return ip; }
    int getPDAState() const { return pdaState; }
    
private:
    std::vector<std::string> stack;
    int ip;
    bool done;
    int pdaState; // 0 = reading, 1 = accept
    std::vector<Token> tokens;
};

#endif // PARSER_H