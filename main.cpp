// main.cpp
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
#include <numeric>
using namespace std;

// ---------------------------
// Backend: Lexer / DFA / Parser
// ---------------------------
enum LabelKind { L_EPS, L_CHAR, L_DIGIT, L_LETTER, L_ALNUM_UNDERSCORE };
struct NFATrans { int to; LabelKind kind; char ch; NFATrans(int t=0, LabelKind k=L_EPS, char c=0):to(t),kind(k),ch(c){} };
struct NFAState { int id; vector<NFATrans> trans; NFAState(int i=0):id(i){} };
struct NFAFragment { int start, accept; NFAFragment(int s=0,int a=0):start(s),accept(a){} };
struct FullNFA { vector<NFAState> states; int start=-1; unordered_map<int,int> acceptToken; int newState(){int id=states.size(); states.emplace_back(id); return id;} };

bool labelMatches(LabelKind kind, char c, char expectedChar=0){
    switch(kind){
        case L_CHAR: return c==expectedChar;
        case L_DIGIT: return isdigit((unsigned char)c);
        case L_LETTER: return isalpha((unsigned char)c);
        case L_ALNUM_UNDERSCORE: return isalnum((unsigned char)c)||c=='_';
        default: return false;
    }
}

NFAFragment makeAtomic(FullNFA &nfa, LabelKind kind, char ch=0){
    int s=nfa.newState(), a=nfa.newState();
    nfa.states[s].trans.emplace_back(a,kind,ch);
    return {s,a};
}
NFAFragment concatFrag(FullNFA &nfa,const NFAFragment &a,const NFAFragment &b){
    nfa.states[a.accept].trans.emplace_back(b.start,L_EPS,0);
    return {a.start,b.accept};
}
NFAFragment unionFrag(FullNFA &nfa,const NFAFragment&a,const NFAFragment&b){
    int s=nfa.newState(),x=nfa.newState();
    nfa.states[s].trans.emplace_back(a.start,L_EPS,0);
    nfa.states[s].trans.emplace_back(b.start,L_EPS,0);
    nfa.states[a.accept].trans.emplace_back(x,L_EPS,0);
    nfa.states[b.accept].trans.emplace_back(x,L_EPS,0);
    return {s,x};
}
NFAFragment starFrag(FullNFA &nfa,const NFAFragment &f){
    int s=nfa.newState(), a=nfa.newState();
    nfa.states[s].trans.emplace_back(f.start,L_EPS,0);
    nfa.states[s].trans.emplace_back(a,L_EPS,0);
    nfa.states[f.accept].trans.emplace_back(f.start,L_EPS,0);
    nfa.states[f.accept].trans.emplace_back(a,L_EPS,0);
    return {s,a};
}
NFAFragment optFrag(FullNFA &nfa,const NFAFragment &f){
    int s=nfa.newState(), a=nfa.newState();
    nfa.states[s].trans.emplace_back(f.start,L_EPS,0);
    nfa.states[s].trans.emplace_back(a,L_EPS,0);
    nfa.states[f.accept].trans.emplace_back(a,L_EPS,0);
    return {s,a};
}

// Tokens
enum TokenID { TK_ID=1, TK_NUMBER, TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_LPAREN, TK_RPAREN, TK_WS };
vector<string> tokenNames = {"", "ID","NUMBER","+","-","*","/","(",")","WS"};

FullNFA buildCombinedNFA(){
    FullNFA nfa; nfa.start=nfa.newState();
    auto insert=[&](NFAFragment f,int tk){nfa.states[nfa.start].trans.emplace_back(f.start,L_EPS,0);nfa.acceptToken[f.accept]=tk;};
    
    // ID: letter (alnum|_)*
    auto letter=makeAtomic(nfa,L_LETTER); 
    auto alnum=makeAtomic(nfa,L_ALNUM_UNDERSCORE);
    insert(concatFrag(nfa,letter,starFrag(nfa,alnum)),TK_ID);
    
    // NUMBER: digit+ (. digit+)?
    // This accepts: 123, 3.14, 0.5, etc.
    auto digit=makeAtomic(nfa,L_DIGIT);
    auto digitPlus = concatFrag(nfa, digit, starFrag(nfa, makeAtomic(nfa,L_DIGIT))); // digit+
    
    auto dot = makeAtomic(nfa,L_CHAR,'.');
    auto fractional = concatFrag(nfa, dot, concatFrag(nfa, makeAtomic(nfa,L_DIGIT), starFrag(nfa, makeAtomic(nfa,L_DIGIT)))); // . digit+
    auto optFractional = optFrag(nfa, fractional); // (. digit+)?
    
    auto numberFrag = concatFrag(nfa, digitPlus, optFractional); // digit+ (. digit+)?
    insert(numberFrag, TK_NUMBER);
    
    // Operators
    insert(makeAtomic(nfa,L_CHAR,'+'),TK_PLUS);
    insert(makeAtomic(nfa,L_CHAR,'-'),TK_MINUS);
    insert(makeAtomic(nfa,L_CHAR,'*'),TK_STAR);
    insert(makeAtomic(nfa,L_CHAR,'/'),TK_SLASH);
    insert(makeAtomic(nfa,L_CHAR,'('),TK_LPAREN);
    insert(makeAtomic(nfa,L_CHAR,')'),TK_RPAREN);
    
    // Whitespace: ( | \t)*
    insert(starFrag(nfa,unionFrag(nfa,makeAtomic(nfa,L_CHAR,' '),makeAtomic(nfa,L_CHAR,'\t'))),TK_WS);
    
    return nfa;
}

struct DFAState { int id=0; unordered_map<char,int> trans; bool accept=false; vector<int> tokens; set<int> nfaStates; };
set<int> epsClosure(const FullNFA &nfa,const set<int>&in){set<int>res=in;vector<int>st(in.begin(),in.end());while(!st.empty()){int s=st.back();st.pop_back();for(auto&t:nfa.states[s].trans)if(t.kind==L_EPS&&!res.count(t.to)){res.insert(t.to);st.push_back(t.to);}}return res;}
set<int> moveVia(const FullNFA &nfa,const set<int>&S,char c){set<int>res;for(int s:S)for(auto&t:nfa.states[s].trans)if(labelMatches(t.kind,c,t.ch))res.insert(t.to);return res;}
vector<char> allChars(){vector<char>v;for(int i=0;i<128;i++)v.push_back((char)i);return v;}
vector<DFAState> subsetConstruct(const FullNFA &nfa){
    vector<DFAState>dfa;map<set<int>,int>id;queue<set<int>>q;set<int>s0=epsClosure(nfa,{nfa.start});id[s0]=0;dfa.push_back({0});dfa[0].nfaStates=s0;
    for(int s:s0) if(nfa.acceptToken.count(s)){dfa[0].accept=true;dfa[0].tokens.push_back(nfa.acceptToken.at(s));}
    q.push(s0); auto chars=allChars();
    while(!q.empty()){
        auto S=q.front(); q.pop(); int sid=id[S];
        for(char c:chars){
            auto mv=moveVia(nfa,S,c); if(mv.empty()) continue;
            auto U=epsClosure(nfa,mv); if(!id.count(U)){
                int nid=dfa.size(); id[U]=nid; dfa.push_back({nid}); dfa[nid].nfaStates=U;
                for(int s:U) if(nfa.acceptToken.count(s)){dfa[nid].accept=true; dfa[nid].tokens.push_back(nfa.acceptToken.at(s));}
                q.push(U);
            }
            dfa[sid].trans[c]=id[U];
        }
    }
    return dfa;
}

struct Token { int id; string lexeme; int pos; };
vector<Token> tokenize(const vector<DFAState>&dfa,const string&in){
    vector<Token>out;int n=in.size(),pos=0;
    while(pos<n){
        int s=0,last=-1,lastPos=pos,cur=pos;
        while(cur<n){char c=in[cur];auto it=dfa[s].trans.find(c);if(it==dfa[s].trans.end())break; s=it->second; if(dfa[s].accept){last=s;lastPos=cur+1;} cur++;}
        if(last==-1) return {};
        vector<int>cand=dfa[last].tokens; sort(cand.begin(),cand.end());
        int tk=cand.front(); string lex=in.substr(pos,lastPos-pos);
        if(tk!=TK_WS) out.push_back({tk,lex,pos});
        pos=lastPos;
    }
    out.push_back({0,"$", (int)in.size()});
    return out;
}

// ---------------------------
// Parser
// ---------------------------
struct Production { string lhs; vector<string> rhs; };
vector<Production> prods; map<pair<string,string>,int> table; string startSym="E";

string tokenToTerm(int id){
    switch(id){case TK_ID: return "ID"; case TK_NUMBER: return "NUMBER"; case TK_PLUS: return "+"; case TK_MINUS: return "-";
        case TK_STAR: return "*"; case TK_SLASH: return "/"; case TK_LPAREN: return "("; case TK_RPAREN: return ")"; case 0: return "$"; default: return "";}
}
bool isTerminal(const string&s){return s=="$"||s=="ID"||s=="NUMBER"||s=="+"||s=="-"||s=="*"||s=="/"||s=="("||s==")";}
void fillGrammar(){
    prods={{"E",{"T","E'"}},{"E'",{"+","T","E'"}},{"E'",{"-","T","E'"}},{"E'",{"ε"}},
           {"T",{"F","T'"}},{"T'",{"*","F","T'"}},{"T'",{"/","F","T'"}},{"T'",{"ε"}},
           {"F",{"+","F"}},{"F",{"-","F"}},{"F",{"(","E",")"}},{"F",{"ID"}},{"F",{"NUMBER"}}};
    vector<string>look={"+","-","(","ID","NUMBER"};
    for(auto&t:look) table[{"E",t}]=0;
    table[{"E'","+"}]=1; table[{"E'","-"}]=2; table[{"E'",")"}]=3; table[{"E'","$"}]=3;
    for(auto&t:look) table[{"T",t}]=4;
    table[{"T'","*"}]=5; table[{"T'","/"}]=6; for(string t:{"+","-","$",")"}) table[{"T'",t}]=7;
    table[{"F","+"}]=8; table[{"F","-"}]=9; table[{"F","("}]=10; table[{"F","ID"}]=11; table[{"F","NUMBER"}]=12;
}

// ---------------------------
// Qt GUI - Enhanced Visualization
// ---------------------------
class AutomataView : public QGraphicsView {
    Q_OBJECT

private:
    void fitSceneInView() {
        if (!scene() || scene()->items().isEmpty()) return;
        QRectF rect = scene()->itemsBoundingRect().adjusted(-100, -100, 100, 100);
        fitInView(rect, Qt::KeepAspectRatio);
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QGraphicsView::resizeEvent(event);
        fitSceneInView(); // automatically adjust when window resizes
    }

public:
    AutomataView(QWidget* parent = nullptr) : QGraphicsView(parent) {
        setScene(new QGraphicsScene(this));
        setRenderHint(QPainter::Antialiasing);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    void buildFromDFA(const vector<DFAState>& dfa_) {
        dfa = dfa_;
        scene()->clear();
        nodes.clear();
        edges.clear();
        edgeLabels.clear();
        if(dfa.empty()) return;

        // BFS for layers
        map<int,int> layer;
        queue<int> q;
        set<int> visited;
        q.push(0); layer[0]=0; visited.insert(0);

        while(!q.empty()) {
            int s = q.front(); q.pop();
            int l = layer[s];
            for(auto &kv : dfa[s].trans){
                int t = kv.second;
                if(!visited.count(t)){
                    visited.insert(t);
                    layer[t] = l+1;
                    q.push(t);
                }
            }
        }

        map<int, vector<int>> layers;
        int maxLayer=0;
        for(auto &p:layer){
            layers[p.second].push_back(p.first);
            maxLayer = max(maxLayer, p.second);
        }

        int W=1600, H=800;
        int verticalSpacing = std::max(150, H / (maxLayer + 2)); // Minimum 150px between layers

        // Create nodes with improved positioning
        for (auto& lv : layers) {
            int y = verticalSpacing * (lv.first + 1);
            int nStates = lv.second.size();
            double horizontalSpacing = std::max(150.0, W / (nStates + 1.0)); // Minimum 200px between nodes

            for (int i = 0; i < nStates; i++) {
                double x = horizontalSpacing * (i + 1);
                int stateId = lv.second[i];

                // Create state circle
               QGraphicsEllipseItem* e = scene()->addEllipse(x - 30, double(y) - 30, 60, 60);
                e->setPen(QPen(Qt::black, 2));
                e->setBrush(QBrush(Qt::white));

                // Double circle for accept states
                if (dfa[stateId].accept) {
                    QGraphicsEllipseItem* inner = scene()->addEllipse(x - 25, double(y) - 25, 50, 50, QPen(Qt::black, 2));
                    e->setBrush(QBrush(QColor(144, 238, 144))); // Light green
                }

                // State label with token info
                QString labelText = QString("q%1").arg(stateId);
                if(dfa[stateId].accept && !dfa[stateId].tokens.empty()) {
                    labelText += "\n[";
                    for(size_t j = 0; j < dfa[stateId].tokens.size(); j++) {
                        if(j > 0) labelText += ",";
                        labelText += QString::fromStdString(tokenNames[dfa[stateId].tokens[j]]);
                    }
                    labelText += "]";
                }

                QGraphicsTextItem* t = scene()->addText(labelText);
                QFont font = t->font();
                font.setPointSize(9);
                font.setBold(true);
                t->setFont(font);
                QRectF bounds = t->boundingRect();
                t->setPos(x - bounds.width()/2, double(y) - bounds.height()/2);

                nodes[stateId] = {x, double(y), e, t};
            }
        }

        // Add start arrow
        if(nodes.count(0)) {
            auto& startNode = nodes[0];
            double arrowStartX = startNode.x - 60;
            double arrowStartY = startNode.y;
            QLineF line(arrowStartX, arrowStartY, startNode.x - 32, startNode.y);
            scene()->addLine(line, QPen(Qt::darkBlue, 3));
            drawArrowHead(arrowStartX + 20, arrowStartY, startNode.x - 32, startNode.y, Qt::darkBlue);
            
            QGraphicsTextItem* startLabel = scene()->addText("START");
            QFont font = startLabel->font();
            font.setPointSize(8);
            font.setBold(true);
            startLabel->setFont(font);
            startLabel->setDefaultTextColor(Qt::darkBlue);
            startLabel->setPos(arrowStartX - 10, arrowStartY - 25);
        }

        // Group transitions by (from, to) pair
        map<pair<int,int>, vector<char>> edgeMap;
        for(int i=0;i<dfa.size();i++)
            for(auto &kv:dfa[i].trans)
                edgeMap[{i, kv.second}].push_back(kv.first);

        // Draw edges with better labels
        for(auto &kv:edgeMap){
            int fromId = kv.first.first;
            int toId = kv.first.second;
            auto &chars = kv.second;

            auto &from = nodes[fromId];
            auto &to = nodes[toId];

            QPainterPath path;
            QPointF labelPos;
            
            if(fromId==toId){
                // Self-loop positioned above the node
                double loopRadius = 35;
                path.moveTo(from.x - 10, from.y - 25);
                path.arcTo(from.x - loopRadius, from.y - 25 - loopRadius*2, loopRadius*2, loopRadius*2, -30, 240);
                labelPos = QPointF(from.x, from.y - 80);
            } else {
                // Straight line between nodes
                double dx = to.x - from.x;
                double dy = to.y - from.y;
                double dist = sqrt(dx*dx + dy*dy);
                
                // Adjust start and end points to node boundary
                double angle = atan2(dy, dx);
                double radius = 30; // Node radius
                QPointF start(from.x + radius * cos(angle), from.y + radius * sin(angle));
                QPointF end(to.x - radius * cos(angle), to.y - radius * sin(angle));
                
                // Straight line
                path.moveTo(start);
                path.lineTo(end);
                
                // Label offset to the side to avoid edge overlap
                double midX = (start.x() + end.x())/2;
                double midY = (start.y() + end.y())/2;
                // Offset perpendicular to the edge
                double perpAngle = angle + M_PI/2;
                labelPos = QPointF(midX + 20*cos(perpAngle), midY + 20*sin(perpAngle));
            }

            QPen edgePen(QColor(80, 80, 80), 2);
            QGraphicsPathItem* item = scene()->addPath(path, edgePen);
            edges.push_back(item);

            // Draw arrow head
            QPointF pEnd = path.pointAtPercent(0.95);
            QPointF pBefore = path.pointAtPercent(0.85);
            drawArrowHead(pBefore.x(), pBefore.y(), pEnd.x(), pEnd.y(), QColor(80, 80, 80));

            // Create compact label
            QString labelText = formatEdgeLabel(chars);
            QGraphicsTextItem* lbl = scene()->addText(labelText);
            QFont font = lbl->font();
            font.setPointSize(8);
            lbl->setFont(font);
            lbl->setDefaultTextColor(Qt::blue);
            
            // Position label near the curve
            QRectF lblBounds = lbl->boundingRect();
            lbl->setPos(labelPos.x() - lblBounds.width()/2, labelPos.y() - lblBounds.height()/2);
            
            edgeLabels.push_back(lbl);
        }

        scene()->setSceneRect(scene()->itemsBoundingRect());
        fitSceneInView();  // dynamic fit
    }

    void highlightState(int id){
        // Reset all nodes
        for(auto &p:nodes){
            int stateId = p.first;
            auto &n = p.second;
            bool isAccept = dfa[stateId].accept;
            n.e->setBrush(isAccept ? QBrush(QColor(144, 238, 144)) : QBrush(Qt::white));
            n.e->setPen(QPen(Qt::black, 2));
        }
        
        // Highlight active state
        if(id >= 0 && nodes.count(id)){
            nodes[id].e->setBrush(QBrush(QColor(255, 255, 100))); // Yellow
            nodes[id].e->setPen(QPen(Qt::red, 3));
        }
        activeState = id;
    }

    void highlightTransition(int fromId, int toId, char c) {
        // Reset all edges
        for(auto* edge : edges) {
            edge->setPen(QPen(QColor(80, 80, 80), 2));
        }
        
        // Find and highlight the specific transition
        // This is a simplified version - you might want to store edge info for precise highlighting
        if(nodes.count(fromId) && nodes.count(toId)) {
            highlightState(toId);
        }
    }

private:
    struct Node { double x,y; QGraphicsEllipseItem* e; QGraphicsTextItem* t; };
    map<int, Node> nodes;
    vector<QGraphicsPathItem*> edges;
    vector<QGraphicsTextItem*> edgeLabels;
    int activeState = -1;
    vector<DFAState> dfa;

    QString formatEdgeLabel(const vector<char>& chars) {
        if(chars.empty()) return "";
        
        // Sort and categorize characters
        vector<char> sorted = chars;
        sort(sorted.begin(), sorted.end());
        
        // Check for character classes
        set<char> charSet(sorted.begin(), sorted.end());
        
        bool hasAllDigits = true;
        for(char c = '0'; c <= '9'; c++) {
            if(!charSet.count(c)) { hasAllDigits = false; break; }
        }
        
        bool hasAllLower = true;
        for(char c = 'a'; c <= 'z'; c++) {
            if(!charSet.count(c)) { hasAllLower = false; break; }
        }
        
        bool hasAllUpper = true;
        for(char c = 'A'; c <= 'Z'; c++) {
            if(!charSet.count(c)) { hasAllUpper = false; break; }
        }
        
        QStringList parts;
        if(hasAllDigits && hasAllLower && hasAllUpper && charSet.count('_')) {
            return "[alnum_]";
        } else if(hasAllDigits && hasAllLower && hasAllUpper) {
            return "[alnum]";
        } else if(hasAllLower && hasAllUpper) {
            return "[a-zA-Z]";
        } else if(hasAllDigits) {
            return "[0-9]";
        } else if(hasAllLower) {
            return "[a-z]";
        } else if(hasAllUpper) {
            return "[A-Z]";
        }
        
        // Otherwise, show individual characters (limit to reasonable number)
        QString result;
        for(size_t i = 0; i < sorted.size() && i < 10; i++) {
            char c = sorted[i];
            if(c == ' ') result += "␣";
            else if(c == '\t') result += "⇥";
            else if(c == '\n') result += "↵";
            else result += QChar(c);
            
            if(i < sorted.size() - 1) result += ",";
        }
        if(sorted.size() > 10) result += "...";
        return result;
    }

    void drawArrowHead(double x1, double y1, double x2, double y2, QColor color = QColor(80, 80, 80)){
        QLineF line(QPointF(x1,y1), QPointF(x2,y2));
        double angle = std::atan2(-line.dy(), line.dx());
        const double arrowSize = 12;
        QPointF p1 = line.p2() - QPointF(arrowSize*cos(angle+M_PI/6), -arrowSize*sin(angle+M_PI/6));
        QPointF p2 = line.p2() - QPointF(arrowSize*cos(angle-M_PI/6), -arrowSize*sin(angle-M_PI/6));
        QPolygonF arrowHead;
        arrowHead << line.p2() << p1 << p2;
        scene()->addPolygon(arrowHead, QPen(color), QBrush(color));
    }
};

// ---------------------------
// MainWindow
// ---------------------------
class MainWindow:public QWidget{
    Q_OBJECT
public:
    MainWindow(QWidget*p=nullptr):QWidget(p){
        setWindowTitle("Expression Automata Visualizer");
        input=new QLineEdit();
        input->setPlaceholderText("Enter expression (e.g., x+y*2)");
        
        auto *lexBtn=new QPushButton("Lex"),*parseBtn=new QPushButton("Parse");
        auto *animBtn=new QPushButton("Animate DFA"),*stopBtn=new QPushButton("Stop");
        auto *resetBtn=new QPushButton("Reset DFA"),*clearBtn=new QPushButton("Clear Output");
        auto *stepParseBtn=new QPushButton("Step Parse"),*resetParseBtn=new QPushButton("Reset Parse");
        
        tokensBox=new QListWidget(); 
        stackBox=new QListWidget(); 
        trace=new QTextEdit(); 
        trace->setReadOnly(true);
        
        // Input display for animation
        inputDisplay = new QLabel();
        inputDisplay->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; font-family: monospace; font-size: 12pt; }");
        inputDisplay->setAlignment(Qt::AlignLeft);

        auto *top=new QHBoxLayout(); 
        top->addWidget(new QLabel("Expression:")); 
        top->addWidget(input);
        top->addWidget(lexBtn); 
        top->addWidget(parseBtn); 
        top->addWidget(clearBtn);

        auto *dfaL=new QVBoxLayout();
        QLabel* dfaTitle = new QLabel("<b style='font-size:14pt; color:#2C5F2D;'>DFA Visualization (Regular Language - Lexical Analysis)</b>");
        dfaTitle->setTextFormat(Qt::RichText);
        dfaL->addWidget(dfaTitle);
        dfaL->addWidget(inputDisplay);
        view=new AutomataView(); 
        view->setMinimumHeight(400);
        dfaL->addWidget(view);
        
        auto *dfaBtns=new QHBoxLayout(); 
        dfaBtns->addWidget(animBtn); 
        dfaBtns->addWidget(stopBtn); 
        dfaBtns->addWidget(resetBtn); 
        dfaL->addLayout(dfaBtns);

        auto *right=new QVBoxLayout();
        QLabel* tokenTitle = new QLabel("<b>Tokens (Lexical Analysis Output):</b>");
        tokenTitle->setTextFormat(Qt::RichText);
        right->addWidget(tokenTitle); 
        right->addWidget(tokensBox);

        // ADD THIS ENTIRE BLOCK:
        QLabel* grammarTitle = new QLabel("<b>Regular Grammars (Token Definitions):</b>");
        grammarTitle->setTextFormat(Qt::RichText);
        right->addWidget(grammarTitle);

        grammarDisplay = new QTextEdit();
        grammarDisplay->setReadOnly(true);
        grammarDisplay->setMaximumHeight(150);
        grammarDisplay->setStyleSheet("QTextEdit { background-color: #F5F5DC; font-family: 'Courier New'; font-size: 9pt; }");
        grammarDisplay->setHtml(
            "<b>ID:</b><br>"
            "&nbsp;&nbsp;Identifier → Letter IdentifierTail<br>"
            "&nbsp;&nbsp;IdentifierTail → Letter IdentifierTail | Digit IdentifierTail | _ IdentifierTail | ε<br>"
            "&nbsp;&nbsp;Letter → a|b|c|...|z|A|B|...|Z<br>"
            "&nbsp;&nbsp;Digit → 0|1|2|...|9<br><br>"
            
            "<b>NUMBER:</b><br>"
            "&nbsp;&nbsp;Number → Digits FractionalPart<br>"
            "&nbsp;&nbsp;Digits → Digit Digits | Digit<br>"
            "&nbsp;&nbsp;FractionalPart → . Digits | ε<br><br>"
            
            "<b>OPERATORS:</b><br>"
            "&nbsp;&nbsp;Plus → +<br>"
            "&nbsp;&nbsp;Minus → -<br>"
            "&nbsp;&nbsp;Star → *<br>"
            "&nbsp;&nbsp;Slash → /<br>"
            "&nbsp;&nbsp;LParen → (<br>"
            "&nbsp;&nbsp;RParen → )<br><br>"
            
            "<b>WHITESPACE:</b><br>"
            "&nbsp;&nbsp;WS → Space WS | Tab WS | ε"
        );
        right->addWidget(grammarDisplay);

        QLabel* pdaTitle = new QLabel("<b style='font-size:12pt; color:#8B4513;'>PDA (Context-Free Grammar - Syntactic Analysis)</b>");
        pdaTitle->setTextFormat(Qt::RichText);
        right->addWidget(pdaTitle);

        // ADD PDA VISUALIZATION
        pdaView = new QGraphicsView();
        pdaView->setScene(new QGraphicsScene(pdaView));
        pdaView->setRenderHint(QPainter::Antialiasing);
        pdaView->setMaximumHeight(180);
        pdaView->setStyleSheet("QGraphicsView { background-color: #FFF8DC; border: 2px solid #8B4513; }");
        right->addWidget(pdaView);

        QLabel* stackLabel = new QLabel("<b>PDA Stack:</b>");
        stackLabel->setTextFormat(Qt::RichText);
        right->addWidget(stackLabel);
        right->addWidget(stackBox);

        auto *pBtns=new QHBoxLayout(); 
        pBtns->addWidget(stepParseBtn); 
        pBtns->addWidget(resetParseBtn); 
        right->addLayout(pBtns);
        right->addWidget(new QLabel("Trace:")); 
        right->addWidget(trace);

        auto *main=new QHBoxLayout(this); 
        auto *left=new QVBoxLayout(); 
        left->addLayout(top); 
        left->addLayout(dfaL); 
        main->addLayout(left,2); 
        main->addLayout(right,1);

        nfa=buildCombinedNFA(); 
        dfa=subsetConstruct(nfa); 
        view->buildFromDFA(dfa); 
        fillGrammar();

        connect(lexBtn,&QPushButton::clicked,this,&MainWindow::onLex);
        connect(parseBtn,&QPushButton::clicked,this,&MainWindow::onParse);
        connect(animBtn,&QPushButton::clicked,this,&MainWindow::onAnimateDFA);
        connect(stopBtn,&QPushButton::clicked,this,&MainWindow::onStopDFA);
        connect(resetBtn,&QPushButton::clicked,this,&MainWindow::onResetDFA);
        connect(clearBtn,&QPushButton::clicked,this,&MainWindow::onClearOutput);
        connect(stepParseBtn,&QPushButton::clicked,this,&MainWindow::onStepParse);
        connect(resetParseBtn,&QPushButton::clicked,this,&MainWindow::onResetParse);

        timer=new QTimer(this); 
        connect(timer,&QTimer::timeout,this,&MainWindow::dfaStepTimer);
        resetDFA(); 
        resetParse();
        drawPDA();
    }

private slots:
    void onLex(){
        tokensBox->clear(); trace->clear(); cur=input->text().toStdString();
        tokens=tokenize(dfa,cur);
        if(tokens.empty()){ trace->append("❌ Lexical error."); return; }
        for(size_t i=0;i<tokens.size();i++){
            if(tokens[i].id==0) tokensBox->addItem(QString("%1: EOF").arg(i));
            else tokensBox->addItem(QString("%1: %2 '%3'").arg(i).arg(QString::fromStdString(tokenNames[tokens[i].id])).arg(QString::fromStdString(tokens[i].lexeme)));
        }
        trace->append("✅ Lexing complete.");
        updateInputDisplay();
        resetDFA();
    }

    void onParse(){
        trace->clear(); 
        if(tokens.empty()){ trace->append("⚠ Please lex first."); return; }
        bool ok=parseAll(tokens); 
        trace->append(ok ? "✅ Parse Accepted" : "❌ Parse Rejected");
    }

    void onAnimateDFA(){ 
        if(tokens.empty()){ trace->append("⚠ Please lex first."); return; }
        
        // Build token-by-token animation sequence
        animationSteps.clear();
        currentTokenIndex = 0;
        
        for(const auto& token : tokens) {
            if(token.id == 0) break; // Skip EOF
            
            string lexeme = token.lexeme;
            int state = 0;
            
            // Add steps for this token
            for(size_t i = 0; i < lexeme.size(); i++) {
                char c = lexeme[i];
                auto it = dfa[state].trans.find(c);
                if(it == dfa[state].trans.end()) break;
                
                int nextState = it->second;
                animationSteps.push_back({
                    state, nextState, c, token.pos + (int)i, 
                    QString::fromStdString(tokenNames[token.id])
                });
                state = nextState;
            }
            
            // Add reset step (back to q0 for next token)
            if(&token != &tokens.back() - 1) { // Not the last token
                animationSteps.push_back({state, 0, '\0', -1, "RESET"});
            }
        }
        
        animationStep = 0;
        trace->append("▶ Starting token-by-token DFA animation...");
        trace->append(QString("Total steps: %1").arg(animationSteps.size()));
        timer->start(600); 
    }
    
    void onStopDFA(){ 
        timer->stop(); 
        trace->append("⏸ Animation stopped."); 
    }
    
    void onResetDFA(){ 
        timer->stop(); 
        resetDFA(); 
        trace->append("🔄 DFA reset."); 
        updateInputDisplay();
    }
    
    void dfaStepTimer(){
        if(animationStep >= (int)animationSteps.size()){ 
            trace->append("✅ Token recognition complete!"); 
            timer->stop(); 
            return; 
        }

        auto& step = animationSteps[animationStep];
        
        if(step.ch == '\0') {
            // Reset step
            view->highlightState(0);
            trace->append("🔄 DFA Reset → q0 (ready for next token)");
            dfaState = 0;
            dfaPos = 0;
        } else {
            // Normal transition
            dfaState = step.toState;
            dfaPos = step.inputPos;
            view->highlightState(dfaState);
            updateInputDisplay();

            QString charDisplay = (step.ch == ' ') ? "␣" : (step.ch == '\t') ? "⇥" : QString(QChar(step.ch));
            
            // Check if we're at an accept state
            QString stateInfo = QString("q%1").arg(dfaState);
            if(dfa[dfaState].accept && !dfa[dfaState].tokens.empty()) {
                stateInfo += " ✓";
            }
            
            trace->append(QString("q%1 --[%2]--> %3 | Token: %4")
                .arg(step.fromState)
                .arg(charDisplay)
                .arg(stateInfo)
                .arg(step.tokenName));
        }
        
        animationStep++;
    }

    void onStepParse(){ 
        if(tokens.empty()){ trace->append("⚠ Please lex first."); return; } 
        if(done) return; 
        if(stepParse()) trace->append("✅ Parse accepted."); 
    }
    
    void onResetParse(){ 
        resetParse(); 
        trace->append("🔄 Parser reset."); 
    }
    
    void onClearOutput(){ 
        trace->clear(); 
        tokensBox->clear(); 
        stackBox->clear(); 
    }

    void drawPDA() {
        QGraphicsScene* scene = pdaView->scene();
        scene->clear();
        
        // Define positions
        double startX = 50, readX = 200, acceptX = 350;
        double y = 90;
        double radius = 30;
        
        // Draw q0 (Reading state)
        QGraphicsEllipseItem* q0 = scene->addEllipse(readX - radius, y - radius, 2*radius, 2*radius, 
            QPen(Qt::black, 2), QBrush(QColor(173, 216, 230))); // Light blue
        QGraphicsTextItem* q0Text = scene->addText("q0\n(Reading)");
        QFont font = q0Text->font();
        font.setPointSize(8);
        font.setBold(true);
        q0Text->setFont(font);
        QRectF q0Bounds = q0Text->boundingRect();
        q0Text->setPos(readX - q0Bounds.width()/2, y - q0Bounds.height()/2);
        
        // Draw q1 (Accept state) - double circle
        QGraphicsEllipseItem* q1Outer = scene->addEllipse(acceptX - radius, y - radius, 2*radius, 2*radius, 
            QPen(Qt::black, 2), QBrush(QColor(144, 238, 144))); // Light green
        scene->addEllipse(acceptX - radius + 4, y - radius + 4, 2*radius - 8, 2*radius - 8, QPen(Qt::black, 2));
        QGraphicsTextItem* q1Text = scene->addText("q1\n(Accept)");
        q1Text->setFont(font);
        QRectF q1Bounds = q1Text->boundingRect();
        q1Text->setPos(acceptX - q1Bounds.width()/2, y - q1Bounds.height()/2);
        
        // Draw START arrow to q0
        QLineF startArrow(startX, y, readX - radius - 5, y);
        scene->addLine(startArrow, QPen(Qt::darkBlue, 2));
        // Arrow head
        QPointF p1 = startArrow.p2() - QPointF(10, 5);
        QPointF p2 = startArrow.p2() - QPointF(10, -5);
        QPolygonF arrowHead;
        arrowHead << startArrow.p2() << p1 << p2;
        scene->addPolygon(arrowHead, QPen(Qt::darkBlue), QBrush(Qt::darkBlue));
        
        QGraphicsTextItem* startLabel = scene->addText("START");
        QFont startFont = startLabel->font();
        startFont.setPointSize(7);
        startFont.setBold(true);
        startLabel->setFont(startFont);
        startLabel->setDefaultTextColor(Qt::darkBlue);
        startLabel->setPos(startX - 5, y - 25);
        
        // Draw self-loop on q0 (for reading input)
        QPainterPath loopPath;
        loopPath.moveTo(readX - 10, y - radius);
        loopPath.arcTo(readX - 35, y - radius - 50, 50, 50, -30, 240);
        scene->addPath(loopPath, QPen(QColor(100, 100, 100), 2));
        
        QGraphicsTextItem* loopLabel = scene->addText("Input symbol,\nStack top →\nStack push/pop");
        QFont loopFont = loopLabel->font();
        loopFont.setPointSize(7);
        loopLabel->setFont(loopFont);
        loopLabel->setDefaultTextColor(Qt::blue);
        loopLabel->setPos(readX - 25, y - 75);
        
        // Draw transition q0 -> q1
        QLineF transLine(readX + radius, y, acceptX - radius, y);
        scene->addLine(transLine, QPen(QColor(80, 80, 80), 2));
        // Arrow head
        QPointF t1 = transLine.p2() - QPointF(10, 5);
        QPointF t2 = transLine.p2() - QPointF(10, -5);
        QPolygonF transArrow;
        transArrow << transLine.p2() << t1 << t2;
        scene->addPolygon(transArrow, QPen(QColor(80, 80, 80)), QBrush(QColor(80, 80, 80)));
        
        QGraphicsTextItem* transLabel = scene->addText("$, $ → ε");
        QFont transFont = transLabel->font();
        transFont.setPointSize(7);
        transLabel->setFont(transFont);
        transLabel->setDefaultTextColor(Qt::blue);
        transLabel->setPos((readX + acceptX)/2 - 20, y - 30);
        
        scene->setSceneRect(scene->itemsBoundingRect().adjusted(-10, -10, 10, 10));
    }

private:
    QLineEdit* input;
    QListWidget* tokensBox; 
    QListWidget* stackBox; 
    QTextEdit* trace; 
    QTextEdit* grammarDisplay;
    AutomataView* view;
    QGraphicsView* pdaView; 
    QTimer* timer;
    QLabel* inputDisplay;
    
    FullNFA nfa; 
    vector<DFAState> dfa; 
    vector<Token> tokens; 
    string cur;
    int dfaState=0, dfaPos=0; 
    bool done=false; 
    vector<string> stack; 
    int ip=0;
    
    // Animation structures
    struct AnimationStep {
        int fromState;
        int toState;
        char ch;
        int inputPos;
        QString tokenName;
    };
    vector<AnimationStep> animationSteps;
    int animationStep = 0;
    int currentTokenIndex = 0;

    void resetDFA(){ 
        dfaState=0; 
        dfaPos=0; 
        view->highlightState(dfaState); 
    }
    
    void resetParse(){ 
        stack={"$","E"}; 
        ip=0; 
        done=false; 
        updateStack(); 
    }
    
    void updateStack(){ 
        stackBox->clear(); 
        for(int i=stack.size()-1;i>=0;i--) 
            stackBox->addItem(QString::fromStdString(stack[i])); 
    }
    
    void updateInputDisplay() {
        if(cur.empty()) {
            inputDisplay->setText("");
            return;
        }
        
        QString html = "<html><body style='font-family: monospace; font-size: 12pt;'>";
        html += "<b>Input:</b> ";
        
        for(int i = 0; i < (int)cur.size(); i++) {
            char c = cur[i];
            QString charStr = (c == ' ') ? "␣" : (c == '\t') ? "⇥" : QString(QChar(c));
            
            if(i < dfaPos) {
                // Already consumed - gray
                html += QString("<span style='color: #888;'>%1</span>").arg(charStr);
            } else if(i == dfaPos && animationStep < (int)animationSteps.size()) {
                // Current character - highlighted
                html += QString("<span style='background-color: yellow; font-weight: bold; padding: 2px;'>%1</span>").arg(charStr);
            } else {
                // Not yet consumed - black
                html += charStr;
            }
        }
        
        html += QString(" | <b>State:</b> q%1").arg(dfaState);
        
        // Show current token being recognized
        if(animationStep < (int)animationSteps.size() && animationSteps[animationStep].ch != '\0') {
            html += QString(" | <b>Recognizing:</b> %1").arg(animationSteps[animationStep].tokenName);
        }
        
        html += "</body></html>";
        
        inputDisplay->setText(html);
    }
    
    bool parseAll(const vector<Token>&t){ 
        resetParse(); 
        while(!done){ 
            if(!stepParse()) return false; 
        } 
        return true; 
    }
    
    bool stepParse(){
        if(done) return true;
        if(stack.empty()){ done=true; return true; }
        
        string top=stack.back(); 
        int id=tokens[ip].id; 
        string term=tokenToTerm(id);
        
        if(top=="$" && term=="$"){ 
            done=true; 
            trace->append("✅ ACCEPT"); 
            return true; 
        }
        
        if(isTerminal(top)){
            if(top==term){ 
                stack.pop_back(); 
                ip++; 
                updateStack(); 
                return true; 
            }
            trace->append(QString("❌ Syntax error: expected %1, got %2")
                .arg(QString::fromStdString(top))
                .arg(QString::fromStdString(term)));
            done=true; 
            return false;
        }
        
        auto it=table.find({top,term}); 
        if(it==table.end()){ 
            trace->append(QString("❌ No parse rule for (%1, %2)")
                .arg(QString::fromStdString(top))
                .arg(QString::fromStdString(term))); 
            done=true; 
            return false; 
        }
        
        int pid=it->second; 
        stack.pop_back(); 
        auto &rhs=prods[pid].rhs; 
        
        if(!(rhs.size()==1 && rhs[0]=="ε")) {
            for(int i=rhs.size()-1;i>=0;i--) 
                stack.push_back(rhs[i]);
        }
        
        QString production = QString::fromStdString(top) + " → ";
        for(size_t i = 0; i < rhs.size(); i++) {
            if(i > 0) production += " ";
            production += QString::fromStdString(rhs[i]);
        }
        trace->append(production);
        
        updateStack(); 
        return true;
    }
};

// ---------------------------
// main()
int main(int argc,char**argv){
    QApplication app(argc,argv);
    MainWindow w; 
    w.resize(1400,800); 
    w.show();
    return app.exec();
}

#include "main.moc"