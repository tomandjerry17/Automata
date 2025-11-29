#include "validator.h"

ValidationResult ExpressionValidator::validate(const std::vector<Token> &tokens) {
    ValidationResult result = {true, "", -1};
    
    if(tokens.empty() || (tokens.size() == 1 && tokens[0].id == 0)) {
        result.valid = false;
        result.error = "Empty expression";
        return result;
    }
    
    // Check 1: Balanced parentheses
    if(!checkBalancedParentheses(tokens, result.error, result.errorPosition)) {
        result.valid = false;
        return result;
    }
    
    // Check 2: No adjacent operators (like ++ or +-)
    if(!checkAdjacentOperators(tokens, result.error, result.errorPosition)) {
        result.valid = false;
        return result;
    }
    
    // Check 3: Operator placement (can't start/end with binary operator)
    if(!checkOperatorPlacement(tokens, result.error, result.errorPosition)) {
        result.valid = false;
        return result;
    }
    
    // Check 4: Unary operators must be in parentheses
    if(!checkUnaryOperatorsInParens(tokens, result.error, result.errorPosition)) {
        result.valid = false;
        return result;
    }
    
    return result;
}

bool ExpressionValidator::checkBalancedParentheses(const std::vector<Token> &tokens, 
                                                    std::string &error, int &pos) {
    int balance = 0;
    int openPos = -1;
    
    for(size_t i = 0; i < tokens.size(); i++) {
        if(tokens[i].id == 0) continue; // Skip EOF
        
        if(tokens[i].id == TK_LPAREN) {
            balance++;
            if(openPos == -1) openPos = i;
        } else if(tokens[i].id == TK_RPAREN) {
            balance--;
            if(balance < 0) {
                error = "Unmatched closing parenthesis ')'";
                pos = i;
                return false;
            }
        }
    }
    
    if(balance > 0) {
        error = "Unmatched opening parenthesis '('";
        pos = openPos;
        return false;
    }
    
    return true;
}

bool ExpressionValidator::checkAdjacentOperators(const std::vector<Token> &tokens, 
                                                  std::string &error, int &pos) {
    for(size_t i = 0; i < tokens.size() - 1; i++) {
        if(tokens[i].id == 0 || tokens[i].id == TK_WS) continue;
        if(tokens[i+1].id == 0 || tokens[i+1].id == TK_WS) continue;
        
        // Check for two adjacent operators (excluding parentheses)
        bool currIsOp = (tokens[i].id == TK_PLUS || tokens[i].id == TK_MINUS || 
                        tokens[i].id == TK_STAR || tokens[i].id == TK_SLASH);
        bool nextIsOp = (tokens[i+1].id == TK_PLUS || tokens[i+1].id == TK_MINUS || 
                        tokens[i+1].id == TK_STAR || tokens[i+1].id == TK_SLASH);
        
        if(currIsOp && nextIsOp) {
            error = "Adjacent operators '" + tokens[i].lexeme + tokens[i+1].lexeme + 
                   "' are not allowed";
            pos = i;
            return false;
        }
    }
    
    return true;
}

bool ExpressionValidator::checkOperatorPlacement(const std::vector<Token> &tokens, 
                                                  std::string &error, int &pos) {
    // Find first non-whitespace token
    int firstReal = -1;
    int lastReal = -1;
    
    for(size_t i = 0; i < tokens.size(); i++) {
        if(tokens[i].id != 0 && tokens[i].id != TK_WS) {
            if(firstReal == -1) firstReal = i;
            lastReal = i;
        }
    }
    
    if(firstReal == -1) {
        error = "Empty expression";
        pos = 0;
        return false;
    }
    
    // Check if starts with binary operator (not allowed at top level)
    int firstId = tokens[firstReal].id;
    if(firstId == TK_PLUS || firstId == TK_STAR || firstId == TK_SLASH) {
        error = "Expression cannot start with operator '" + tokens[firstReal].lexeme + "'";
        pos = firstReal;
        return false;
    }
    
    // Check if ends with operator
    int lastId = tokens[lastReal].id;
    if(lastId == TK_PLUS || lastId == TK_MINUS || 
       lastId == TK_STAR || lastId == TK_SLASH) {
        error = "Expression cannot end with operator '" + tokens[lastReal].lexeme + "'";
        pos = lastReal;
        return false;
    }
    
    return true;
}

bool ExpressionValidator::checkUnaryOperatorsInParens(const std::vector<Token> &tokens, 
                                                       std::string &error, int &pos) {
    // Check: unary +/- must be inside parentheses
    // Valid: (−3), (+5), a+(−b)
    // Invalid: −3, +5 (at top level or after operators without parens)
    
    int parenDepth = 0;
    int prevNonWS = -1; // Previous non-whitespace token index
    
    for(size_t i = 0; i < tokens.size(); i++) {
        int id = tokens[i].id;
        
        if(id == TK_WS || id == 0) continue;
        
        if(id == TK_LPAREN) {
            parenDepth++;
            prevNonWS = i;
            continue;
        }
        
        if(id == TK_RPAREN) {
            parenDepth--;
            prevNonWS = i;
            continue;
        }
        
        // Check if this +/- is unary
        if(id == TK_PLUS || id == TK_MINUS) {
            bool isUnary = false;
            
            if(prevNonWS == -1) {
                // First token - it's unary
                isUnary = true;
            } else {
                int prevId = tokens[prevNonWS].id;
                // Unary if previous is operator or opening paren
                if(prevId == TK_PLUS || prevId == TK_MINUS || 
                   prevId == TK_STAR || prevId == TK_SLASH ||
                   prevId == TK_LPAREN) {
                    isUnary = true;
                }
            }
            
            // If unary and NOT inside parentheses, reject
            if(isUnary && parenDepth == 0) {
                error = "Unary operator '" + tokens[i].lexeme + 
                       "' must be enclosed in parentheses, e.g., (" + tokens[i].lexeme + "3)";
                pos = i;
                return false;
            }
        }
        
        prevNonWS = i;
    }
    
    return true;
}

bool ExpressionValidator::isBinaryOperator(int tokenId) {
    return tokenId == TK_PLUS || tokenId == TK_MINUS || 
           tokenId == TK_STAR || tokenId == TK_SLASH;
}

bool ExpressionValidator::isUnaryOperator(int tokenId) {
    return tokenId == TK_PLUS || tokenId == TK_MINUS;
}

bool ExpressionValidator::isOperand(int tokenId) {
    return tokenId == TK_ID || tokenId == TK_NUMBER;
}