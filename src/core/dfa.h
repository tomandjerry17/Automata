#ifndef DFA_H
#define DFA_H

#include <unordered_map>
#include <vector>
#include <set>

struct DFAState { 
    int id = 0; 
    std::unordered_map<char, int> trans; 
    bool accept = false; 
    std::vector<int> tokens; 
    std::set<int> nfaStates; 
};

#endif // DFA_H