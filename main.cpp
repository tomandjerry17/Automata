// main.cpp
// Build with CMakeLists above (Qt6 Widgets). C++17.
// Single-file GUI demo: DFA/PDA visualizer for simple arithmetic expression validator.

#include <QtWidgets>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
using namespace std;

/* ---------------------------
   Backend: NFA / DFA / Lexer
   (adapted / simplified from the earlier console example)
   --------------------------- */

enum LabelKind { L_EPS, L_CHAR, L_DIGIT, L_LETTER, L_ALNUM_UNDERSCORE };

struct NFATrans { int to; LabelKind kind; char ch; NFATrans(int t=0, LabelKind k=L_EPS, char c=0):to(t),kind(k),ch(c){} };
struct NFAState { int id; vector<NFATrans> trans; NFAState(int i=0):id(i){} };
struct NFAFragment { int start, accept; NFAFragment(int s=0,int a=0):start(s),accept(a){} };

struct FullNFA {
    vector<NFAState> states;
    int start = -1;
    unordered_map<int,int> acceptToken;
    int newState(){ int id = (int)states.size(); states.emplace_back(id); return id; }
};

bool labelMatches(LabelKind kind, char c, char expectedChar=0) {
    switch(kind){
        case L_CHAR: return c == expectedChar;
        case L_DIGIT: return isdigit((unsigned char)c);
        case L_LETTER: return isalpha((unsigned char)c);
        case L_ALNUM_UNDERSCORE: return isalnum((unsigned char)c) || c == '_';
        default: return false;
    }
}

NFAFragment makeAtomic(FullNFA &nfa, LabelKind kind, char ch=0){
    int s = nfa.newState(), a = nfa.newState();
    nfa.states[s].trans.emplace_back(a, kind, ch);
    return {s,a};
}
NFAFragment concatFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b){
    nfa.states[a.accept].trans.emplace_back(b.start, L_EPS, 0);
    return {a.start, b.accept};
}
NFAFragment unionFrag(FullNFA &nfa, const NFAFragment &a, const NFAFragment &b){
    int s = nfa.newState(), x = nfa.newState();
    nfa.states[s].trans.emplace_back(a.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(b.start, L_EPS, 0);
    nfa.states[a.accept].trans.emplace_back(x, L_EPS, 0);
    nfa.states[b.accept].trans.emplace_back(x, L_EPS, 0);
    return {s,x};
}
NFAFragment starFrag(FullNFA &nfa, const NFAFragment &f){
    int s = nfa.newState(), a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s,a};
}
NFAFragment plusFrag(FullNFA &nfa, const NFAFragment &f){
    auto sf = starFrag(nfa, f);
    return concatFrag(nfa, f, sf);
}
NFAFragment optFrag(FullNFA &nfa, const NFAFragment &f){
    int s = nfa.newState(), a = nfa.newState();
    nfa.states[s].trans.emplace_back(f.start, L_EPS, 0);
    nfa.states[s].trans.emplace_back(a, L_EPS, 0);
    nfa.states[f.accept].trans.emplace_back(a, L_EPS, 0);
    return {s,a};
}

// tokens
enum TokenID { TK_ID=1, TK_NUMBER, TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_LPAREN, TK_RPAREN, TK_WS };
vector<string> tokenNames = {"", "ID","NUMBER","+","-","*","/","("," )","WS"};

FullNFA buildCombinedNFA(){
    FullNFA nfa; nfa.start = nfa.newState();
    auto insert = [&](NFAFragment frag, int tok){
        nfa.states[nfa.start].trans.emplace_back(frag.start, L_EPS, 0);
        nfa.acceptToken[frag.accept] = tok;
    };

    auto letter = makeAtomic(nfa, L_LETTER);
    auto alnumu = makeAtomic(nfa, L_ALNUM_UNDERSCORE);
    auto idfrag = concatFrag(nfa, letter, starFrag(nfa, alnumu));
    insert(idfrag, TK_ID);

    auto digit = makeAtomic(nfa, L_DIGIT);
    auto digitsPlus = plusFrag(nfa, digit);
    auto dot = makeAtomic(nfa, L_CHAR, '.');
    auto frac = concatFrag(nfa, dot, digitsPlus);
    auto numfrag = concatFrag(nfa, digitsPlus, optFrag(nfa, frac));
    insert(numfrag, TK_NUMBER);

    insert(makeAtomic(nfa, L_CHAR, '+'), TK_PLUS);
    insert(makeAtomic(nfa, L_CHAR, '-'), TK_MINUS);
    insert(makeAtomic(nfa, L_CHAR, '*'), TK_STAR);
    insert(makeAtomic(nfa, L_CHAR, '/'), TK_SLASH);
    insert(makeAtomic(nfa, L_CHAR, '('), TK_LPAREN);
    insert(makeAtomic(nfa, L_CHAR, ')'), TK_RPAREN);

    // whitespace: space, tab, newline, carriage return
    auto sp = makeAtomic(nfa, L_CHAR, ' ');
    auto tab = makeAtomic(nfa, L_CHAR, '\t');
    auto nl = makeAtomic(nfa, L_CHAR, '\n');
    auto cr = makeAtomic(nfa, L_CHAR, '\r');
    auto s1 = unionFrag(nfa, sp, tab);
    auto s2 = unionFrag(nfa, nl, cr);
    auto sAll = unionFrag(nfa, s1, s2);
    auto wsp = plusFrag(nfa, sAll);
    insert(wsp, TK_WS);

    return nfa;
}

// DFA
struct DFAState { int id=0; unordered_map<char,int> trans; bool accept=false; vector<int> tokens; set<int> nfaStates; };
set<int> epsClosure(const FullNFA &nfa, const set<int>& in){
    set<int> res = in; vector<int> st(in.begin(), in.end());
    while(!st.empty()){
        int s = st.back(); st.pop_back();
        for(auto &t : nfa.states[s].trans){
            if (t.kind == L_EPS && !res.count(t.to)){ res.insert(t.to); st.push_back(t.to); }
        }
    }
    return res;
}
set<int> moveVia(const FullNFA &nfa, const set<int>& S, char c){
    set<int> res;
    for(int s : S){
        for(auto &t : nfa.states[s].trans){
            if (t.kind == L_CHAR && labelMatches(t.kind, c, t.ch)) res.insert(t.to);
            if (t.kind == L_DIGIT && labelMatches(t.kind, c)) res.insert(t.to);
            if (t.kind == L_LETTER && labelMatches(t.kind, c)) res.insert(t.to);
            if (t.kind == L_ALNUM_UNDERSCORE && labelMatches(t.kind, c)) res.insert(t.to);
        }
    }
    return res;
}
vector<char> allChars(){ vector<char> v; for(int i=0;i<128;i++) v.push_back((char)i); return v; }

vector<DFAState> subsetConstruct(const FullNFA &nfa){
    vector<DFAState> dfa; map<set<int>, int> idMap; queue<set<int>> q;
    set<int> s0 = { nfa.start };
    s0 = epsClosure(nfa, s0);
    idMap[s0] = 0; dfa.push_back(DFAState()); dfa[0].id = 0; dfa[0].nfaStates = s0;
    for(int s : s0) if (nfa.acceptToken.count(s)) { dfa[0].accept = true; dfa[0].tokens.push_back(nfa.acceptToken.at(s)); }
    q.push(s0);
    auto chars = allChars();
    while(!q.empty()){
        auto S = q.front(); q.pop();
        int sid = idMap[S];
        for(char c : chars){
            auto moved = moveVia(nfa, S, c);
            if (moved.empty()) continue;
            auto U = epsClosure(nfa, moved);
            if (!idMap.count(U)){
                int nid = dfa.size();
                idMap[U] = nid;
                dfa.push_back(DFAState());
                dfa[nid].id = nid; dfa[nid].nfaStates = U;
                for(int s : U) if (nfa.acceptToken.count(s)) { dfa[nid].accept = true; dfa[nid].tokens.push_back(nfa.acceptToken.at(s)); }
                q.push(U);
            }
            dfa[sid].trans[c] = idMap[U];
        }
    }
    return dfa;
}

struct Token { int id; string lexeme; int pos; };
vector<Token> tokenize(const vector<DFAState> &dfa, const string &input){
    vector<Token> out; int n = input.size(), pos = 0;
    while(pos < n){
        int state = 0; int lastAccept = -1; int lastAcceptPos = pos;
        int cur = pos;
        while(cur < n){
            char c = input[cur];
            auto it = dfa[state].trans.find(c);
            if (it == dfa[state].trans.end()) break;
            state = it->second;
            if (dfa[state].accept){ lastAccept = state; lastAcceptPos = cur+1; }
            cur++;
        }
        if (lastAccept == -1){
            // lexical error
            return {};
        }
        // choose token (first by sorted token id)
        vector<int> cand = dfa[lastAccept].tokens;
        sort(cand.begin(), cand.end());
        int tk = cand.front();
        string lex = input.substr(pos, lastAcceptPos - pos);
        if (tk != TK_WS) out.push_back({tk, lex, pos});
        pos = lastAcceptPos;
    }
    out.push_back({0,"$", (int)input.size()});
    return out;
}

/* ---------------------------
   Parser (LL(1)) — same grammar as before
   E -> T E'
   E' -> + T E' | - T E' | ε
   T -> F T'
   T' -> * F T' | / F T' | ε
   F -> + F | - F | ( E ) | ID | NUMBER
   --------------------------- */

struct Production { string lhs; vector<string> rhs; Production(string l="", vector<string> r=vector<string>()):lhs(l),rhs(r){} };

vector<Production> prods;
map<pair<string,string>, int> parseTable;
string startSymbol = "E";

string tokenToTerm(int id){
    if (id == TK_ID) return "ID";
    if (id == TK_NUMBER) return "NUMBER";
    if (id == TK_PLUS) return "+";
    if (id == TK_MINUS) return "-";
    if (id == TK_STAR) return "*";
    if (id == TK_SLASH) return "/";
    if (id == TK_LPAREN) return "(";
    if (id == TK_RPAREN) return ")";
    if (id == 0) return "$";
    return "";
}

bool isTerminalSym(const string &s){
    if (s == "$") return true;
    if (s == "ID" || s == "NUMBER") return true;
    if (s == "+"|| s=="-"|| s=="*"|| s=="/"|| s=="("|| s==")") return true;
    return false;
}

void fillGrammar(){
    prods.clear();
    parseTable.clear();
    prods.emplace_back("E", vector<string>{"T","E'"}); //0
    prods.emplace_back("E'", vector<string>{"+","T","E'"}); //1
    prods.emplace_back("E'", vector<string>{"-","T","E'"}); //2
    prods.emplace_back("E'", vector<string>{"ε"}); //3
    prods.emplace_back("T", vector<string>{"F","T'"}); //4
    prods.emplace_back("T'", vector<string>{"*","F","T'"}); //5
    prods.emplace_back("T'", vector<string>{"/","F","T'"}); //6
    prods.emplace_back("T'", vector<string>{"ε"}); //7
    prods.emplace_back("F", vector<string>{"+","F"}); //8 unary +
    prods.emplace_back("F", vector<string>{"-","F"}); //9 unary -
    prods.emplace_back("F", vector<string>{"(","E",")"}); //10
    prods.emplace_back("F", vector<string>{"ID"}); //11
    prods.emplace_back("F", vector<string>{"NUMBER"}); //12

    vector<string> look = {"+","-","(","ID","NUMBER"};
    for(auto &t: look) parseTable[{"E",t}] = 0;
    parseTable[{"E'","+"}] = 1; parseTable[{"E'","-"}] = 2;
    parseTable[{"E'",")"}] = 3; parseTable[{"E'","$"}] = 3;
    for(auto &t: look) parseTable[{"T",t}] = 4;
    parseTable[{"T'","*"}] = 5; parseTable[{"T'","/"}] = 6;
    parseTable[{"T'","+"}] = 7; parseTable[{"T'","-"}] = 7; parseTable[{"T'",")"}] = 7; parseTable[{"T'","$"}] = 7;
    parseTable[{"F","+"}] = 8; parseTable[{"F","-"}] = 9; parseTable[{"F","("}] = 10; parseTable[{"F","ID"}] = 11; parseTable[{"F","NUMBER"}] = 12;
}

/* ---------------------------
   Qt GUI: MainWindow with controls and a QGraphicsView to draw DFA
   --------------------------- */

class AutomataView : public QGraphicsView {
    Q_OBJECT
public:
    AutomataView(QWidget *parent=nullptr) : QGraphicsView(parent) {
        setScene(new QGraphicsScene(this));
        setRenderHint(QPainter::Antialiasing);
    }

    // Build a simple layout of DFA states (circle nodes) from DFA vector
    void buildFromDFA(const vector<DFAState> &dfa) {
        scene()->clear();
        nodes.clear();
        int n = (int)dfa.size();
        if (n==0) return;
        // place nodes in circle
        int W = 600, H = 300;
        double cx = W/2.0, cy = H/2.0, R = min(W,H)/2.5;
        for(int i=0;i<n;i++){
            double ang = 2.0*M_PI*i/n;
            double x = cx + R*cos(ang);
            double y = cy + R*sin(ang);
            QGraphicsEllipseItem *e = scene()->addEllipse(x-25,y-25,50,50, QPen(Qt::black), QBrush(Qt::white));
            QGraphicsTextItem *t = scene()->addText(QString::number(i));
            t->setPos(x-8,y-12);
            nodes.push_back({x,y,e,t});
        }
        // edges (draw simple straight lines)
        for(int i=0;i<n;i++){
            for(auto &kv : dfa[i].trans){
                int j = kv.second;
                // draw arrow from i to j if unique small offset
                double x1 = nodes[i].x, y1 = nodes[i].y;
                double x2 = nodes[j].x, y2 = nodes[j].y;
                QGraphicsLineItem *line = scene()->addLine(x1,y1,x2,y2, QPen(Qt::darkGray));
                Q_UNUSED(line);
            }
        }
        fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    void highlightState(int id) {
        // reset all
        for(auto &n : nodes){ n.ellipse->setBrush(QBrush(Qt::white)); }
        if (id >=0 && id < (int)nodes.size()){
            nodes[id].ellipse->setBrush(QBrush(Qt::yellow));
        }
    }

private:
    struct Node { double x,y; QGraphicsEllipseItem* ellipse; QGraphicsTextItem* label; };
    vector<Node> nodes;
};

/* MainWindow */
class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent=nullptr) : QWidget(parent) {
        setWindowTitle("Expr Validator - DFA/PDA Visualizer");

        // UI elements
        inputEdit = new QLineEdit();
        lexButton = new QPushButton("Lex");
        parseButton = new QPushButton("Parse");
        stepDfaButton = new QPushButton("Step DFA");
        resetDfaButton = new QPushButton("Reset DFA");
        stepParseButton = new QPushButton("Step Parse");
        resetParseButton = new QPushButton("Reset Parse");

        tokenList = new QListWidget();
        traceBox = new QTextEdit(); traceBox->setReadOnly(true);
        stackList = new QListWidget();

        auto *topLayout = new QHBoxLayout();
        topLayout->addWidget(new QLabel("Expression:"));
        topLayout->addWidget(inputEdit);
        topLayout->addWidget(lexButton);
        topLayout->addWidget(parseButton);

        auto *dfaLayout = new QVBoxLayout();
        automataView = new AutomataView();
        automataView->setMinimumSize(640, 320);
        dfaLayout->addWidget(automataView);
        auto *dfaBtns = new QHBoxLayout();
        dfaBtns->addWidget(stepDfaButton);
        dfaBtns->addWidget(resetDfaButton);
        dfaLayout->addLayout(dfaBtns);

        auto *rightLayout = new QVBoxLayout();
        rightLayout->addWidget(new QLabel("Tokens:"));
        rightLayout->addWidget(tokenList);
        rightLayout->addWidget(new QLabel("Parser Stack:"));
        rightLayout->addWidget(stackList);
        auto *parseBtns = new QHBoxLayout();
        parseBtns->addWidget(stepParseButton);
        parseBtns->addWidget(resetParseButton);
        rightLayout->addLayout(parseBtns);
        rightLayout->addWidget(new QLabel("Trace:"));
        rightLayout->addWidget(traceBox);

        auto *mainLayout = new QHBoxLayout(this);
        auto *left = new QVBoxLayout();
        left->addLayout(topLayout);
        left->addLayout(dfaLayout);
        mainLayout->addLayout(left, 3);
        mainLayout->addLayout(rightLayout, 1);

        // build NFA & DFA once
        nfa = buildCombinedNFA();
        dfaStates = subsetConstruct(nfa);
        automataView->buildFromDFA(dfaStates);

        fillGrammar();

        // connect
        connect(lexButton, &QPushButton::clicked, this, &MainWindow::onLex);
        connect(parseButton, &QPushButton::clicked, this, &MainWindow::onParse);
        connect(stepDfaButton, &QPushButton::clicked, this, &MainWindow::onStepDfa);
        connect(resetDfaButton, &QPushButton::clicked, this, &MainWindow::onResetDfa);
        connect(stepParseButton, &QPushButton::clicked, this, &MainWindow::onStepParse);
        connect(resetParseButton, &QPushButton::clicked, this, &MainWindow::onResetParse);

        resetDfa();
        resetParse();
    }

private slots:
    void onLex(){
        tokenList->clear();
        traceBox->clear();
        curInput = inputEdit->text().toStdString();
        tokens = tokenize(dfaStates, curInput);
        if (tokens.empty()){
            traceBox->append("Lexical error.\n");
            return;
        }
        for(size_t i=0;i<tokens.size();++i){
            auto &t = tokens[i];
            if (t.id==0) tokenList->addItem(QString("%1: EOF").arg((int)i));
            else tokenList->addItem(QString("%1: %2  '%3'").arg((int)i).arg(QString::fromStdString(tokenNames[t.id])).arg(QString::fromStdString(t.lexeme)));
        }
        traceBox->append("Lexing completed. Tokens listed.\n");
        // prepare DFA stepping
        resetDfa();
    }
    void onParse(){
        traceBox->clear();
        if (tokens.empty()){ traceBox->append("No tokens: lex first.\n"); return; }
        // do full parse and show full trace in traceBox and stackList final state
        bool ok = parseAll(tokens);
        traceBox->append(ok ? "Parse result: ACCEPTED\n" : "Parse result: REJECTED\n");
    }

    void onStepDfa(){
        // advance one character in DFA visualization
        if (dfaPos > (int)curInput.size()) return;
        if (dfaPos == (int)curInput.size()){
            traceBox->append("DFA at end (EOF)\n");
            automataView->highlightState(-1);
            return;
        }
        // feed next char
        char c = curInput[dfaPos];
        auto it = dfaStates[dfaActiveState].trans.find(c);
        if (it == dfaStates[dfaActiveState].trans.end()){
            traceBox->append(QString("DFA stuck at input pos %1 char '%2'").arg(dfaPos).arg(QChar(c)));
            return;
        }
        dfaActiveState = it->second;
        dfaPos++;
        automataView->highlightState(dfaActiveState);
        traceBox->append(QString("DFA consumed '%1', moved to state %2").arg(QChar(c)).arg(dfaActiveState));
    }

    void onResetDfa(){
        resetDfa();
        traceBox->append("DFA reset.\n");
    }

    void onStepParse(){
        // step one parser action
        if (tokens.empty()){ traceBox->append("No token stream. Please Lex first.\n"); return; }
        if (parseDone) { traceBox->append("Parse already finished.\n"); return; }
        // execute one parser step
        if (parseStep()) {
            traceBox->append("Parse finished: ACCEPT\n");
            parseDone = true;
        }
    }
    void onResetParse(){
        resetParse();
        traceBox->append("Parser reset.\n");
    }

private:
    // UI
    QLineEdit *inputEdit;
    QPushButton *lexButton, *parseButton, *stepDfaButton, *resetDfaButton, *stepParseButton, *resetParseButton;
    QListWidget *tokenList;
    QTextEdit *traceBox;
    QListWidget *stackList;
    AutomataView *automataView;

    // backend objects
    FullNFA nfa;
    vector<DFAState> dfaStates;
    vector<Token> tokens;
    string curInput;

    // DFA stepping state
    int dfaActiveState = 0;
    int dfaPos = 0;

    // Parser stepping
    vector<string> stackSym;
    int ip = 0;
    bool parseDone = false;

    void resetDfa(){
        dfaActiveState = 0;
        dfaPos = 0;
        automataView->highlightState(dfaActiveState);
    }

    void resetParse(){
        stackSym.clear();
        stackSym.push_back("$");
        stackSym.push_back(startSymbol);
        ip = 0;
        parseDone = false;
        updateStackView();
    }

    void updateStackView(){
        stackList->clear();
        for(auto &s : stackSym) stackList->addItem(QString::fromStdString(s));
    }

    bool parseAll(const vector<Token> &tokStream){
        // full parse, produce trace in traceBox
        vector<string> stk; stk.push_back("$"); stk.push_back(startSymbol);
        int p = 0;
        int step = 0;
        while(!stk.empty()){
            step++;
            string top = stk.back();
            string look = tokenToTerm(tokStream[p].id);
            traceBox->append(QString("Step %1: Stack top='%2', lookahead='%3'").arg(step).arg(QString::fromStdString(top)).arg(QString::fromStdString(look)));
            if (isTerminalSym(top)){
                if (top == look){
                    traceBox->append("  Match terminal: pop & consume");
                    stk.pop_back(); p++;
                } else {
                    traceBox->append(QString("  Parse error: expected '%1' but got '%2'").arg(QString::fromStdString(top), QString::fromStdString(look)));
                    return false;
                }
            } else {
                auto key = make_pair(top, look);
                if (parseTable.find(key) == parseTable.end()){
                    traceBox->append(QString("  Parse error: no rule for nonterminal '%1' with lookahead '%2'").arg(QString::fromStdString(top), QString::fromStdString(look)));
                    return false;
                }
                int pi = parseTable[key];
                auto &prod = prods[pi];
                string rhs;
                for(auto &r : prod.rhs) rhs += r + " ";
                traceBox->append(QString("  Apply: %1 -> %2").arg(QString::fromStdString(prod.lhs), QString::fromStdString(rhs)));
                stk.pop_back();
                if (!(prod.rhs.size()==1 && prod.rhs[0]=="ε")){
                    for(auto it = prod.rhs.rbegin(); it!=prod.rhs.rend(); ++it) stk.push_back(*it);
                }
            }
        }
        if (tokenToTerm(tokStream[p].id) == "$") return true;
        return false;
    }

    // parser single-step (returns true if finished & accepted)
    bool parseStep(){
        if (stackSym.empty()) return true;
        string top = stackSym.back();
        string look = tokenToTerm(tokens[ip].id);
        if (isTerminalSym(top)){
            if (top == look){
                traceBox->append(QString("Match terminal '%1'").arg(QString::fromStdString(top)));
                stackSym.pop_back(); ip++;
                updateStackView();
                if (stackSym.empty() && tokenToTerm(tokens[ip].id)=="$") {
                    traceBox->append("Parse accepted.");
                    return true;
                }
                return false;
            } else {
                traceBox->append(QString("Parse error: expected '%1' but got '%2'").arg(QString::fromStdString(top), QString::fromStdString(look)));
                parseDone = true;
                return true;
            }
        } else {
            auto key = make_pair(top, look);
            if (parseTable.find(key) == parseTable.end()){
                traceBox->append(QString("Parse error: no rule for '%1' with lookahead '%2'").arg(QString::fromStdString(top), QString::fromStdString(look)));
                parseDone = true;
                return true;
            }
            int pi = parseTable[key];
            auto &prod = prods[pi];
            QString rhs;
            for(auto &r: prod.rhs) rhs += QString::fromStdString(r) + " ";
            traceBox->append(QString("Apply %1 -> %2").arg(QString::fromStdString(prod.lhs), rhs));
            stackSym.pop_back();
            if (!(prod.rhs.size()==1 && prod.rhs[0]=="ε")){
                for(auto it = prod.rhs.rbegin(); it!=prod.rhs.rend(); ++it) stackSym.push_back(*it);
            }
            updateStackView();
            return false;
        }
    }
};

int main(int argc, char **argv){
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1000,600);
    w.show();
    return app.exec();
}

#include "main.moc"