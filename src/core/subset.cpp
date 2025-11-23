#include "subset.h"
#include <queue>
#include <map>
#include <algorithm>
#include <iostream> // DEBUG

std::set<int> epsClosure(const FullNFA &nfa, const std::set<int> &in) {
    std::set<int> res = in;
    std::vector<int> st(in.begin(), in.end());
    
    while(!st.empty()) {
        int s = st.back();
        st.pop_back();
        for(auto &t : nfa.states[s].trans) {
            if(t.kind == L_EPS && !res.count(t.to)) {
                res.insert(t.to);
                st.push_back(t.to);
            }
        }
    }
    return res;
}

std::set<int> moveVia(const FullNFA &nfa, const std::set<int> &S, char c) {
    std::set<int> res;
    for(int s : S) {
        for(auto &t : nfa.states[s].trans) {
            if(labelMatches(t.kind, c, t.ch)) {
                res.insert(t.to);
            }
        }
    }
    return res;
}

std::vector<char> allChars() {
    std::vector<char> v;
    for(int i = 0; i < 128; i++) {
        v.push_back((char)i);
    }
    return v;
}

std::vector<DFAState> subsetConstruct(const FullNFA &nfa) {
    std::vector<DFAState> dfa;
    std::map<std::set<int>, int> id;
    std::queue<std::set<int>> q;
    
    std::set<int> s0 = epsClosure(nfa, {nfa.start});
    id[s0] = 0;
    dfa.push_back({0});
    dfa[0].nfaStates = s0;
    
    // DEBUG: Print start state info
    std::cout << "=== DFA State 0 (Start) ===" << std::endl;
    std::cout << "NFA states in closure: ";
    for(int s : s0) {
        std::cout << s << " ";
    }
    std::cout << std::endl;
    
    // Check if start state should be accept
    bool startIsAccept = false;
    for(int s : s0) {
        if(nfa.acceptToken.count(s)) {
            dfa[0].accept = true;
            dfa[0].tokens.push_back(nfa.acceptToken.at(s));
            startIsAccept = true;
            std::cout << "  State " << s << " is accept state for token " << nfa.acceptToken.at(s) << std::endl;
        }
    }
    
    if(!startIsAccept) {
        std::cout << "  Start state is NOT an accept state (CORRECT)" << std::endl;
    } else {
        std::cout << "  WARNING: Start state IS an accept state (INCORRECT)" << std::endl;
    }
    
    q.push(s0);
    auto chars = allChars();
    
    int stateCount = 1;
    while(!q.empty()) {
        auto S = q.front();
        q.pop();
        int sid = id[S];
        
        for(char c : chars) {
            auto mv = moveVia(nfa, S, c);
            if(mv.empty()) continue;
            
            auto U = epsClosure(nfa, mv);
            if(!id.count(U)) {
                int nid = dfa.size();
                id[U] = nid;
                dfa.push_back({nid});
                dfa[nid].nfaStates = U;
                
                // DEBUG: Print new state info
                std::cout << "\n=== DFA State " << nid << " ===" << std::endl;
                std::cout << "Created from char: '" << c << "' from state " << sid << std::endl;
                std::cout << "NFA states: ";
                for(int s : U) {
                    std::cout << s << " ";
                }
                std::cout << std::endl;
                
                // Check if this new state should be accept
                bool isAccept = false;
                for(int s : U) {
                    if(nfa.acceptToken.count(s)) {
                        dfa[nid].accept = true;
                        dfa[nid].tokens.push_back(nfa.acceptToken.at(s));
                        isAccept = true;
                        std::cout << "  State " << s << " is accept state for token " << nfa.acceptToken.at(s) << std::endl;
                    }
                }
                
                if(!isAccept) {
                    std::cout << "  This is NOT an accept state" << std::endl;
                }
                
                q.push(U);
                stateCount++;
            }
            
            dfa[sid].trans[c] = id[U];
        }
    }
    
    std::cout << "\n=== DFA CONSTRUCTION COMPLETE ===" << std::endl;
    std::cout << "Total DFA states: " << stateCount << std::endl;
    std::cout << "Accept states: ";
    for(size_t i = 0; i < dfa.size(); i++) {
        if(dfa[i].accept) {
            std::cout << i << " ";
        }
    }
    std::cout << std::endl;
    
    return dfa;
}