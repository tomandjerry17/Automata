#ifndef NFA_H
#define NFA_H

#include <vector>
#include <unordered_map>

enum LabelKind { 
    L_EPS, 
    L_CHAR, 
    L_DIGIT, 
    L_LETTER, 
    L_ALNUM_UNDERSCORE 
};

struct NFATrans { 
    int to; 
    LabelKind kind; 
    char ch; 
    NFATrans(int t = 0, LabelKind k = L_EPS, char c = 0);
};

struct NFAState { 
    int id; 
    std::vector<NFATrans> trans; 
    NFAState(int i = 0);
};

struct NFAFragment { 
    int start, accept; 
    NFAFragment(int s = 0, int a = 0);
};

struct FullNFA { 
    std::vector<NFAState> states; 
    int start = -1; 
    std::unordered_map<int, int> acceptToken;
    
    int newState();
};

bool labelMatches(LabelKind kind, char c, char expectedChar = 0);

#endif // NFA_H