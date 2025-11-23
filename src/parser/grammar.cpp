#include "grammar.h"
#include "core/tokens.h"

std::vector<Production> prods;
std::map<std::pair<std::string, std::string>, int> table;
std::string startSym = "E";

std::string tokenToTerm(int id) {
    switch(id) {
        case TK_ID: return "ID";
        case TK_NUMBER: return "NUMBER";
        case TK_PLUS: return "+";
        case TK_MINUS: return "-";
        case TK_STAR: return "*";
        case TK_SLASH: return "/";
        case TK_LPAREN: return "(";
        case TK_RPAREN: return ")";
        case 0: return "$";
        default: return "";
    }
}

bool isTerminal(const std::string &s) {
    return s == "$" || s == "ID" || s == "NUMBER" || 
           s == "+" || s == "-" || s == "*" || s == "/" || 
           s == "(" || s == ")";
}

void fillGrammar() {
    prods = {
        {"E",  {"T", "E'"}},
        {"E'", {"+", "T", "E'"}},
        {"E'", {"-", "T", "E'"}},
        {"E'", {"ε"}},
        {"T",  {"F", "T'"}},
        {"T'", {"*", "F", "T'"}},
        {"T'", {"/", "F", "T'"}},
        {"T'", {"ε"}},
        {"F",  {"+", "F"}},
        {"F",  {"-", "F"}},
        {"F",  {"(", "E", ")"}},
        {"F",  {"ID"}},
        {"F",  {"NUMBER"}}
    };
    
    std::vector<std::string> look = {"+", "-", "(", "ID", "NUMBER"};
    for(auto &t : look) table[{"E", t}] = 0;
    
    table[{"E'", "+"}] = 1;
    table[{"E'", "-"}] = 2;
    table[{"E'", ")"}] = 3;
    table[{"E'", "$"}] = 3;
    
    for(auto &t : look) table[{"T", t}] = 4;
    
    table[{"T'", "*"}] = 5;
    table[{"T'", "/"}] = 6;
    for(std::string t : {"+", "-", "$", ")"}) table[{"T'", t}] = 7;
    
    table[{"F", "+"}] = 8;
    table[{"F", "-"}] = 9;
    table[{"F", "("}] = 10;
    table[{"F", "ID"}] = 11;
    table[{"F", "NUMBER"}] = 12;
}