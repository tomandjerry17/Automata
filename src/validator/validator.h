#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "core/tokens.h"
#include <vector>
#include <string>

struct ValidationResult {
    bool valid;
    std::string error;
    int errorPosition; // Token index where error occurs
};

class ExpressionValidator {
public:
    static ValidationResult validate(const std::vector<Token> &tokens);
    
private:
    static bool checkBalancedParentheses(const std::vector<Token> &tokens, std::string &error, int &pos);
    static bool checkAdjacentOperators(const std::vector<Token> &tokens, std::string &error, int &pos);
    static bool checkOperatorPlacement(const std::vector<Token> &tokens, std::string &error, int &pos);
    static bool checkUnaryOperatorsInParens(const std::vector<Token> &tokens, std::string &error, int &pos);
    static bool isBinaryOperator(int tokenId);
    static bool isUnaryOperator(int tokenId);
    static bool isOperand(int tokenId);
};

#endif // VALIDATOR_H