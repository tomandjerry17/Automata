#ifndef SUBSET_H
#define SUBSET_H

#include "nfa.h"
#include "dfa.h"
#include <set>
#include <vector>

std::set<int> epsClosure(const FullNFA &nfa, const std::set<int> &in);
std::set<int> moveVia(const FullNFA &nfa, const std::set<int> &S, char c);
std::vector<char> allChars();
std::vector<DFAState> subsetConstruct(const FullNFA &nfa);

#endif // SUBSET_H