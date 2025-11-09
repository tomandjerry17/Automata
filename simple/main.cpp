#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <map>
#include <stack>
#include <iomanip>
using namespace std;

// -----------------------------
//  LEXICAL ANALYZER
// -----------------------------
enum TokenType {
    ID, NUMBER, PLUS, MINUS, STAR, SLASH, LPAREN, RPAREN, END, INVALID
};

struct Token {
    TokenType type;
    string lexeme;
};

vector<Token> tokenize(const string &input) {
    vector<Token> tokens;
    size_t i = 0;
    while (i < input.size()) {
        if (isspace(input[i])) { i++; continue; }

        if (isalpha(input[i])) {
            string id;
            while (i < input.size() && isalnum(input[i])) id += input[i++];
            tokens.push_back({ID, id});
        }
        else if (isdigit(input[i])) {
            string num;
            while (i < input.size() && (isdigit(input[i]) || input[i] == '.')) num += input[i++];
            tokens.push_back({NUMBER, num});
        }
        else {
            char c = input[i++];
            switch (c) {
                case '+': tokens.push_back({PLUS, string(1,c)}); break;
                case '-': tokens.push_back({MINUS, string(1,c)}); break;
                case '*': tokens.push_back({STAR, string(1,c)}); break;
                case '/': tokens.push_back({SLASH, string(1,c)}); break;
                case '(': tokens.push_back({LPAREN, string(1,c)}); break;
                case ')': tokens.push_back({RPAREN, string(1,c)}); break;
                default: tokens.push_back({INVALID, string(1,c)}); break;
            }
        }
    }
    tokens.push_back({END, "$"});
    return tokens;
}

string tokenName(TokenType t) {
    switch (t) {
        case ID: return "ID";
        case NUMBER: return "NUMBER";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case END: return "END";
        default: return "INVALID";
    }
}

// -----------------------------
//  SYNTAX ANALYZER (LL(1))
// -----------------------------
//
// Grammar:
//
// E  → T E'
// E' → + T E' | - T E' | ε
// T  → F T'
// T' → * F T' | / F T' | ε
// F  → + F | - F | (E) | ID | NUMBER
//

map<string, map<string, vector<string>>> parseTable;

void buildParseTable() {
    // E productions
    parseTable["E"]["ID"] = {"T", "E'"};
    parseTable["E"]["NUMBER"] = {"T", "E'"};
    parseTable["E"]["LPAREN"] = {"T", "E'"};
    // ❌ removed "E"]["MINUS"] — handled by F → -F

    // E' productions
    parseTable["E'"]["PLUS"] = {"+", "T", "E'"};
    parseTable["E'"]["MINUS"] = {"-", "T", "E'"};
    parseTable["E'"]["RPAREN"] = {}; // ε
    parseTable["E'"]["END"] = {};    // ε

    // T productions
    parseTable["T"]["ID"] = {"F", "T'"};
    parseTable["T"]["NUMBER"] = {"F", "T'"};
    parseTable["T"]["LPAREN"] = {"F", "T'"};
    // ❌ removed "T"]["MINUS"] — handled by F → -F

    // T' productions
    parseTable["T'"]["PLUS"] = {};   // ε
    parseTable["T'"]["MINUS"] = {};  // ε
    parseTable["T'"]["STAR"] = {"*", "F", "T'"};
    parseTable["T'"]["SLASH"] = {"/", "F", "T'"};
    parseTable["T'"]["RPAREN"] = {}; // ε
    parseTable["T'"]["END"] = {};    // ε

    // F productions
    parseTable["F"]["ID"] = {"ID"};
    parseTable["F"]["NUMBER"] = {"NUMBER"};
    parseTable["F"]["LPAREN"] = {"LPAREN", "E", "RPAREN"};
    parseTable["F"]["PLUS"] = {"+", "F"};
    parseTable["F"]["MINUS"] = {"-", "F"};
}

string tokenToKey(TokenType t) {
    switch (t) {
        case ID: return "ID";
        case NUMBER: return "NUMBER";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case END: return "END";
        default: return "INVALID";
    }
}

bool parse(const vector<Token>& tokens) {
    stack<string> st;
    st.push("$");
    st.push("E");
    size_t ip = 0;

    cout << "\nSyntax Analysis:\n";

    while (!st.empty()) {
        string top = st.top();
        TokenType t = tokens[ip].type;
        string tk = tokenToKey(t);

        // Print stack and current token
        cout << "Stack: ";
        stack<string> temp = st;
        vector<string> rev;
        while (!temp.empty()) { rev.push_back(temp.top()); temp.pop(); }
        for (int i = rev.size()-1; i >= 0; --i) cout << rev[i] << " ";
        cout << " | Input: " << tokens[ip].lexeme << " (" << tk << ")\n";

        if (top == "$" && tk == "END") {
            cout << "\n✅ ACCEPTED: Valid expression.\n";
            return true;
        }

        if (top == tk) {
            st.pop();
            ip++;
        } else if (parseTable.count(top) && parseTable[top].count(tk)) {
            st.pop();
            auto rule = parseTable[top][tk];
            if (!rule.empty()) {
                for (int i = rule.size()-1; i >= 0; --i)
                    st.push(rule[i]);
            } // ε → push nothing
        } else {
            cout << "\n❌ ERROR: Unexpected token '" << tokens[ip].lexeme
                 << "' at position " << ip << "\n";
            return false;
        }
    }

    cout << "\n❌ ERROR: Stack emptied unexpectedly.\n";
    return false;
}

// -----------------------------
//  MAIN
// -----------------------------
int main() {
    buildParseTable();

    string input;
    cout << "Enter an arithmetic expression:\n> ";
    getline(cin, input);

    auto tokens = tokenize(input);

    cout << "\nLexical Analysis:\n";
    for (auto &t : tokens)
        cout << "Token: " << setw(8) << left << tokenName(t.type)
             << " Lexeme: " << t.lexeme << "\n";

    parse(tokens);
    return 0;
}
