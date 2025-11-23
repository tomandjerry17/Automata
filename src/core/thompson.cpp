#include "thompson.h"
#include "tokens.h"

NFAFragment makeAtomic(FullNFA &nfa, LabelKind kind, char ch) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(a, kind, ch);
    return {s, a};
}

NFAFragment concatFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b) {
    nfa.states[a.accept].trans.emplace_back(b.start, L_EPS, 0);
    return {a.start, b.accept};
}

NFAFragment unionFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b) {
    int s = nfa.newState();
    int x = nfa.newState();
    nfa.states[s].trans.emplace_back(a.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(b.start, L_EPS, 0);
    nfa.states[a.accept].trans.emplace_back(x, L_EPS, 0);
    nfa.states[b.accept].trans.emplace_back(x, L_EPS, 0);
    return {s, x};
}

NFAFragment starFrag(FullNFA &nfa, const NFAFragment &f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

NFAFragment optFrag(FullNFA &nfa, const NFAFragment &f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

// Helper to create (a|b)+ (one or more)
NFAFragment plusFrag(FullNFA &nfa, const NFAFragment &f) {
    int s = nfa.newState();
    int a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s, a};
}

FullNFA buildCombinedNFA() {
    FullNFA nfa;
    nfa.start = nfa.newState();
    
    auto insert = [&](NFAFragment f, int tk) {
        nfa.states[nfa.start].trans.emplace_back(f.start, L_EPS, 0);
        nfa.acceptToken[f.accept] = tk;
    };
    
    // ID: letter (alnum|_)*
    auto letter = makeAtomic(nfa, L_LETTER);
    auto alnum = makeAtomic(nfa, L_ALNUM_UNDERSCORE);
    insert(concatFrag(nfa, letter, starFrag(nfa, alnum)), TK_ID);
    
    // NUMBER: digit+ (. digit+)?
    auto digit = makeAtomic(nfa, L_DIGIT);
    auto digitPlus = concatFrag(nfa, digit, starFrag(nfa, makeAtomic(nfa, L_DIGIT)));
    
    auto dot = makeAtomic(nfa, L_CHAR, '.');
    auto fractional = concatFrag(nfa, dot, 
                        concatFrag(nfa, makeAtomic(nfa, L_DIGIT), 
                                  starFrag(nfa, makeAtomic(nfa, L_DIGIT))));
    auto optFractional = optFrag(nfa, fractional);
    auto numberFrag = concatFrag(nfa, digitPlus, optFractional);
    insert(numberFrag, TK_NUMBER);
    
    // Operators
    insert(makeAtomic(nfa, L_CHAR, '+'), TK_PLUS);
    insert(makeAtomic(nfa, L_CHAR, '-'), TK_MINUS);
    insert(makeAtomic(nfa, L_CHAR, '*'), TK_STAR);
    insert(makeAtomic(nfa, L_CHAR, '/'), TK_SLASH);
    insert(makeAtomic(nfa, L_CHAR, '('), TK_LPAREN);
    insert(makeAtomic(nfa, L_CHAR, ')'), TK_RPAREN);
    
    // Whitespace: (space | tab)+ (one or more, NOT zero or more)
    // This prevents empty string from being accepted
    auto space = makeAtomic(nfa, L_CHAR, ' ');
    auto tab = makeAtomic(nfa, L_CHAR, '\t');
    auto ws_union = unionFrag(nfa, space, tab);
    insert(plusFrag(nfa, ws_union), TK_WS);
    
    return nfa;
}