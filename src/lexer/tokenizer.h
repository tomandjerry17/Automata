#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "core/tokens.h"
#include "core/dfa.h"
#include <string>
#include <vector>

std::vector<Token> tokenize(const std::vector<DFAState> &dfa, const std::string &in);

#endif // TOKENIZER_H