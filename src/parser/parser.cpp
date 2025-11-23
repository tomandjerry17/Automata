#include "parser.h"

Parser::Parser() {
    reset();
}

void Parser::reset() {
    stack = {"$", "E"};
    ip = 0;
    done = false;
    pdaState = 0;
}

bool Parser::parseAll(const std::vector<Token> &tkns) {
    tokens = tkns;
    reset();
    while(!done) {
        if(!stepParse(tokens)) return false;
    }
    return true;
}

bool Parser::stepParse(const std::vector<Token> &tkns) {
    if(done) return true;
    if(stack.empty()) { 
        done = true; 
        return true; 
    }
    
    tokens = tkns;
    std::string top = stack.back();
    int id = tokens[ip].id;
    std::string term = tokenToTerm(id);
    
    // Accept condition
    if(top == "$" && term == "$") {
        done = true;
        pdaState = 1;
        return true;
    }
    
    // Terminal matching
    if(isTerminal(top)) {
        if(top == term) {
            stack.pop_back();
            ip++;
            return true;
        }
        done = true;
        return false;
    }
    
    // Non-terminal: lookup production
    auto it = table.find({top, term});
    if(it == table.end()) {
        done = true;
        return false;
    }
    
    int pid = it->second;
    stack.pop_back();
    auto &rhs = prods[pid].rhs;
    
    if(!(rhs.size() == 1 && rhs[0] == "ε")) {
        for(int i = rhs.size() - 1; i >= 0; i--) {
            stack.push_back(rhs[i]);
        }
    }
    
    return true;
}