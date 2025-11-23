#ifndef THOMPSON_H
#define THOMPSON_H

#include "nfa.h"

NFAFragment makeAtomic(FullNFA &nfa, LabelKind kind, char ch = 0);
NFAFragment concatFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b);
NFAFragment unionFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b);
NFAFragment starFrag(FullNFA &nfa, const NFAFragment &f);
NFAFragment optFrag(FullNFA &nfa, const NFAFragment &f);

FullNFA buildCombinedNFA();

#endif // THOMPSON_H