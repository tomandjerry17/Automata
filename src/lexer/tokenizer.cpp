#include "tokenizer.h"
#include <algorithm>

std::vector<Token> tokenize(const std::vector<DFAState> &dfa, const std::string &in) {
    std::vector<Token> out;
    int n = in.size();
    int pos = 0;
    
    while(pos < n) {
        int s = 0;
        int last = -1;
        int lastPos = pos;
        int cur = pos;
        
        // Scan for longest match
        while(cur < n) {
            char c = in[cur];
            auto it = dfa[s].trans.find(c);
            if(it == dfa[s].trans.end()) break;
            
            s = it->second;
            if(dfa[s].accept) {
                last = s;
                lastPos = cur + 1;
            }
            cur++;
        }
        
        if(last == -1) return {}; // Lexical error
        
        // Get token with highest priority
        std::vector<int> cand = dfa[last].tokens;
        std::sort(cand.begin(), cand.end());
        int tk = cand.front();
        std::string lex = in.substr(pos, lastPos - pos);
        
        if(tk != TK_WS) { // Skip whitespace
            out.push_back({tk, lex, pos});
        }
        pos = lastPos;
    }
    
    out.push_back({0, "$", (int)in.size()}); // EOF
    return out;
}