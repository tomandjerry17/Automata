#include "nfa.h"
#include <cctype>

NFATrans::NFATrans(int t, LabelKind k, char c) 
    : to(t), kind(k), ch(c) {}

NFAState::NFAState(int i) : id(i) {}

NFAFragment::NFAFragment(int s, int a) 
    : start(s), accept(a) {}

int FullNFA::newState() {
    int id = states.size();
    states.emplace_back(id);
    return id;
}

bool labelMatches(LabelKind kind, char c, char expectedChar) {
    switch(kind) {
        case L_CHAR: 
            return c == expectedChar;
        case L_DIGIT: 
            return std::isdigit(static_cast<unsigned char>(c));
        case L_LETTER: 
            return std::isalpha(static_cast<unsigned char>(c));
        case L_ALNUM_UNDERSCORE: 
            return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
        default: 
            return false;
    }
}