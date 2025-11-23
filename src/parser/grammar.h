#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <string>
#include <vector>
#include <map>

struct Production { 
    std::string lhs; 
    std::vector<std::string> rhs; 
};

extern std::vector<Production> prods;
extern std::map<std::pair<std::string, std::string>, int> table;
extern std::string startSym;

void fillGrammar();
std::string tokenToTerm(int id);
bool isTerminal(const std::string &s);

#endif // GRAMMAR_H