#include "mainwindow.h"
#include "core/thompson.h"
#include "core/subset.h"
#include "lexer/tokenizer.h"
#include "parser/grammar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <cmath>
#include <QTabWidget>
#include <QScrollArea>
#include <QGroupBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Automata Visualizer - Compiler Front-End");
    
    // Shared input field at the top
    input = new QLineEdit();
    input->setPlaceholderText("Enter expression (e.g., a+b*2)");
    
    auto *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("<b>Expression:</b>"));
    inputLayout->addWidget(input);
    
    // Create tab widget
    tabWidget = new QTabWidget();
    
    // ============================================
    // TAB 1: NFA Construction (Thompson's)
    // ============================================
    QWidget *tab1 = new QWidget();
    QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);
    
    QLabel *tab1Title = new QLabel("<h2 style='color:#2C5F2D;'>Tab 1: NFA Construction (Thompson's Construction)</h2>");
    tab1Title->setTextFormat(Qt::RichText);
    tab1Layout->addWidget(tab1Title);
    
    auto *buildNfaBtn = new QPushButton("Build NFA from Regular Expressions");
    tab1Layout->addWidget(buildNfaBtn);
    
    QLabel *tab1Info = new QLabel(
        "<b>Objective:</b> Convert regular expressions into NFAs using Thompson's construction.<br>"
        "<b>Status:</b> Click 'Build NFA' to visualize individual NFAs for each token type."
    );
    tab1Info->setTextFormat(Qt::RichText);
    tab1Info->setWordWrap(true);
    tab1Layout->addWidget(tab1Info);
    
    // NFA Views for each token type
    QScrollArea *nfaScrollArea = new QScrollArea();
    QWidget *nfaContainer = new QWidget();
    QVBoxLayout *nfaContainerLayout = new QVBoxLayout(nfaContainer);

    // Create individual NFA views
    QStringList tokenLabels = {"ID", "NUMBER", "PLUS (+)", "MINUS (-)", "STAR (*)", "SLASH (/)", "LPAREN (()", "RPAREN ())", "WHITESPACE"};

    for(const QString &label : tokenLabels) {
        NFAView *nfaView = new NFAView();
        nfaViews.push_back(nfaView);
        
        QGroupBox *groupBox = new QGroupBox(QString("Token: %1").arg(label));
        QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);
        groupLayout->addWidget(nfaView);
        
        nfaContainerLayout->addWidget(groupBox);
    }

    nfaScrollArea->setWidget(nfaContainer);
    nfaScrollArea->setWidgetResizable(true);
    tab1Layout->addWidget(nfaScrollArea);
    
    tabWidget->addTab(tab1, "1. NFA Construction");
    
    // ============================================
    // TAB 2: DFA Conversion (Subset Construction)
    // ============================================
    QWidget *tab2 = new QWidget();
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);
    
    QLabel *tab2Title = new QLabel("<h2 style='color:#1E5F8C;'>Tab 2: DFA Conversion (Subset Construction)</h2>");
    tab2Title->setTextFormat(Qt::RichText);
    tab2Layout->addWidget(tab2Title);
    
    auto *lexBtn = new QPushButton("Tokenize (Build DFA)");
    auto *animBtn = new QPushButton("Animate DFA");
    auto *stopBtn = new QPushButton("Stop");
    auto *resetBtn = new QPushButton("Reset");
    auto *clearBtn = new QPushButton("Clear Output");
    
    auto *tab2Btns = new QHBoxLayout();
    tab2Btns->addWidget(lexBtn);
    tab2Btns->addWidget(animBtn);
    tab2Btns->addWidget(stopBtn);
    tab2Btns->addWidget(resetBtn);
    tab2Btns->addWidget(clearBtn);
    tab2Layout->addLayout(tab2Btns);
    
    inputDisplay = new QLabel();
    inputDisplay->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; font-family: monospace; font-size: 11pt; }");
    tab2Layout->addWidget(inputDisplay);
    
    view = new AutomataView();
    view->setMinimumHeight(350);
    tab2Layout->addWidget(view);
    
    // Add DFA info
    dfaInfo = new QLabel();
    dfaInfo->setTextFormat(Qt::RichText);
    dfaInfo->setWordWrap(true);
    dfaInfo->setStyleSheet("QLabel { background-color: #E8F4F8; padding: 10px; border: 1px solid #1E5F8C; border-radius: 5px; }");
    tab2Layout->addWidget(dfaInfo);

    // Tokens and trace side by side
    auto *tab2Bottom = new QHBoxLayout();

    auto *tokensLayout = new QVBoxLayout();
    tokensLayout->addWidget(new QLabel("<b>Tokens:</b>"));
    tokensBox = new QListWidget();
    tokensBox->setMaximumHeight(150);
    tokensLayout->addWidget(tokensBox);
    tab2Bottom->addLayout(tokensLayout);

    auto *traceLayout = new QVBoxLayout();
    traceLayout->addWidget(new QLabel("<b>DFA Animation Trace:</b>"));
    trace = new QTextEdit();
    trace->setReadOnly(true);
    trace->setMaximumHeight(150);
    traceLayout->addWidget(trace);
    tab2Bottom->addLayout(traceLayout);

    tab2Layout->addLayout(tab2Bottom);
    
    tabWidget->addTab(tab2, "2. DFA Conversion");
    
    // ============================================
    // TAB 3: PDA Parser (Syntactic Analysis)
    // ============================================
    QWidget *tab3 = new QWidget();
    QVBoxLayout *tab3Layout = new QVBoxLayout(tab3);
    
    QLabel *tab3Title = new QLabel("<h2 style='color:#8B4513;'>Tab 3: PDA Parser (Syntactic Analysis)</h2>");
    tab3Title->setTextFormat(Qt::RichText);
    tab3Layout->addWidget(tab3Title);
    
    auto *parseBtn = new QPushButton("Parse Expression");
    auto *stepParseBtn = new QPushButton("Step Parse");
    auto *resetParseBtn = new QPushButton("Reset Parser");
    
    auto *tab3Btns = new QHBoxLayout();
    tab3Btns->addWidget(parseBtn);
    tab3Btns->addWidget(stepParseBtn);
    tab3Btns->addWidget(resetParseBtn);
    tab3Layout->addLayout(tab3Btns);

    // ADD THIS: Input display for parser
    parseInputDisplay = new QLabel();
    parseInputDisplay->setStyleSheet("QLabel { background-color: #FFF8DC; padding: 5px; font-family: monospace; font-size: 11pt; border: 1px solid #8B4513; }");
    parseInputDisplay->setAlignment(Qt::AlignLeft);
    tab3Layout->addWidget(parseInputDisplay);
    
    // PDA Visualization
    pdaView = new QGraphicsView();
    pdaView->setScene(new QGraphicsScene(pdaView));
    pdaView->setRenderHint(QPainter::Antialiasing);
    pdaView->setMaximumHeight(200);
    pdaView->setStyleSheet("QGraphicsView { background-color: #FFF8DC; border: 2px solid #8B4513; }");
    tab3Layout->addWidget(pdaView);
    
    // Stack and Grammar side by side
    auto *tab3Middle = new QHBoxLayout();
    
    auto *stackLayout = new QVBoxLayout();
    stackLayout->addWidget(new QLabel("<b>PDA Stack:</b>"));
    stackBox = new QListWidget();
    stackBox->setMaximumHeight(200);
    stackLayout->addWidget(stackBox);
    tab3Middle->addLayout(stackLayout);
    
    auto *grammarLayout = new QVBoxLayout();
    grammarLayout->addWidget(new QLabel("<b>LL(1) Parsing Table:</b>"));

    // Create parsing table
    parsingTableWidget = new QTableWidget();
    parsingTableWidget->setMaximumHeight(200);
    parsingTableWidget->setRowCount(5); // E, E', T, T', F
    parsingTableWidget->setColumnCount(9); // +, -, *, /, (, ), ID, NUM, $

    // Set headers
    QStringList rowHeaders = {"E", "E'", "T", "T'", "F"};
    QStringList colHeaders = {"+", "-", "*", "/", "(", ")", "ID", "NUM", "$"};
    parsingTableWidget->setVerticalHeaderLabels(rowHeaders);
    parsingTableWidget->setHorizontalHeaderLabels(colHeaders);

    // Fill table with productions (simplified view)
    parsingTableWidget->setItem(0, 4, new QTableWidgetItem("E→TE'")); // E, (
    parsingTableWidget->setItem(0, 6, new QTableWidgetItem("E→TE'")); // E, ID
    parsingTableWidget->setItem(0, 7, new QTableWidgetItem("E→TE'")); // E, NUM

    parsingTableWidget->setItem(1, 0, new QTableWidgetItem("E'→+TE'")); // E', +
    parsingTableWidget->setItem(1, 1, new QTableWidgetItem("E'→-TE'")); // E', -
    parsingTableWidget->setItem(1, 5, new QTableWidgetItem("E'→ε")); // E', )
    parsingTableWidget->setItem(1, 8, new QTableWidgetItem("E'→ε")); // E', $

    parsingTableWidget->setItem(2, 4, new QTableWidgetItem("T→FT'")); // T, (
    parsingTableWidget->setItem(2, 6, new QTableWidgetItem("T→FT'")); // T, ID
    parsingTableWidget->setItem(2, 7, new QTableWidgetItem("T→FT'")); // T, NUM

    parsingTableWidget->setItem(3, 2, new QTableWidgetItem("T'→*FT'")); // T', *
    parsingTableWidget->setItem(3, 3, new QTableWidgetItem("T'→/FT'")); // T', /
    parsingTableWidget->setItem(3, 0, new QTableWidgetItem("T'→ε")); // T', +
    parsingTableWidget->setItem(3, 1, new QTableWidgetItem("T'→ε")); // T', -
    parsingTableWidget->setItem(3, 5, new QTableWidgetItem("T'→ε")); // T', )
    parsingTableWidget->setItem(3, 8, new QTableWidgetItem("T'→ε")); // T', $

    parsingTableWidget->setItem(4, 0, new QTableWidgetItem("F→+F")); // F, +
    parsingTableWidget->setItem(4, 1, new QTableWidgetItem("F→-F")); // F, -
    parsingTableWidget->setItem(4, 4, new QTableWidgetItem("F→(E)")); // F, (
    parsingTableWidget->setItem(4, 6, new QTableWidgetItem("F→ID")); // F, ID
    parsingTableWidget->setItem(4, 7, new QTableWidgetItem("F→NUM")); // F, NUM

    parsingTableWidget->resizeColumnsToContents();
    parsingTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    grammarLayout->addWidget(parsingTableWidget);

    // Add CFG productions below table
    grammarDisplay = new QTextEdit();
    grammarDisplay->setReadOnly(true);
    grammarDisplay->setMaximumHeight(120);
    grammarDisplay->setStyleSheet("QTextEdit { background-color: #F5F5DC; font-family: 'Courier New'; font-size: 8pt; }");
    grammarDisplay->setHtml(
        "<b>Context-Free Grammar:</b><br>"
        "E → T E'<br>"
        "E' → + T E' | - T E' | ε<br>"
        "T → F T'<br>"
        "T' → * F T' | / F T' | ε<br>"
        "F → + F | - F | ( E ) | ID | NUMBER"
    );
    grammarLayout->addWidget(grammarDisplay);
    tab3Middle->addLayout(grammarLayout);
    
    tab3Layout->addLayout(tab3Middle);
    
    // Parse trace at bottom
    parseTrace = new QTextEdit();
    parseTrace->setReadOnly(true);
    parseTrace->setMaximumHeight(150);
    parseTrace->setPlaceholderText("Parse trace will appear here...");
    tab3Layout->addWidget(new QLabel("<b>Parse Trace:</b>"));
    tab3Layout->addWidget(parseTrace);
    
    tabWidget->addTab(tab3, "3. PDA Parser");
    
    // ============================================
    // Main Layout
    // ============================================
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(tabWidget);
    
    // ============================================
    // Initialize backend
    // ============================================
    nfa = buildCombinedNFA();
    dfa = subsetConstruct(nfa);
    view->buildFromDFA(dfa);
    fillGrammar();
    
    // ============================================
    // Connect signals
    // ============================================
    connect(buildNfaBtn, &QPushButton::clicked, this, &MainWindow::onBuildNFA);
    
    connect(lexBtn, &QPushButton::clicked, this, &MainWindow::onLex);
    connect(parseBtn, &QPushButton::clicked, this, &MainWindow::onParse);
    connect(animBtn, &QPushButton::clicked, this, &MainWindow::onAnimateDFA);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopDFA);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::onResetDFA);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearOutput);
    connect(stepParseBtn, &QPushButton::clicked, this, &MainWindow::onStepParse);
    connect(resetParseBtn, &QPushButton::clicked, this, &MainWindow::onResetParse);
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::dfaStepTimer);

    pdaTimer = new QTimer(this);
    connect(pdaTimer, &QTimer::timeout, this, &MainWindow::pdaStepTimer);

    
    resetDFA();
    resetParse();
    drawPDA();
}

void MainWindow::updateParseInputDisplay() {
    if(tokens.empty() || cur.empty()) {
        parseInputDisplay->setText("");
        return;
    }
    
    QString html = "<html><body style='font-family: monospace; font-size: 11pt;'>";
    html += "<b>Input:</b> ";
    
    int currentPos = parser.getCurrentPosition();
    
    for(size_t i = 0; i < tokens.size(); i++) {
        QString tokenStr = QString::fromStdString(
            tokens[i].id == 0 ? "$" : tokenNames[tokens[i].id]
        );
        
        if((int)i < currentPos) {
            // Already consumed
            html += QString("<span style='color: #888;'>%1 </span>").arg(tokenStr);
        } else if((int)i == currentPos) {
            // Current token being processed
            html += QString("<span style='background-color: yellow; font-weight: bold; padding: 2px;'>%1</span> ").arg(tokenStr);
        } else {
            // Not yet consumed
            html += tokenStr + " ";
        }
    }
    
    auto stack = parser.getStack();
    html += QString(" | <b>Stack Top:</b> %1").arg(
        stack.empty() ? "$" : QString::fromStdString(stack.back())
    );
    
    html += "</body></html>";
    parseInputDisplay->setText(html);
}

void MainWindow::onBuildNFA() {
    // Build individual NFAs for each token
    FullNFA tempNfa;
    tempNfa.start = tempNfa.newState();
    
    // ID
    auto letter = makeAtomic(tempNfa, L_LETTER);
    auto alnum = makeAtomic(tempNfa, L_ALNUM_UNDERSCORE);
    auto idFrag = concatFrag(tempNfa, letter, starFrag(tempNfa, alnum));
    nfaViews[0]->buildFromNFA(idFrag, tempNfa, "[a-zA-Z][a-zA-Z0-9_]*");
    
    // NUMBER
    FullNFA numNfa;
    numNfa.start = numNfa.newState();
    auto digit = makeAtomic(numNfa, L_DIGIT);
    auto digitPlus = concatFrag(numNfa, digit, starFrag(numNfa, makeAtomic(numNfa, L_DIGIT)));
    nfaViews[1]->buildFromNFA(digitPlus, numNfa, "[0-9]+(\\.[0-9]+)?");
    
    // Simple operators
    FullNFA plusNfa;
    plusNfa.start = plusNfa.newState();
    auto plusFrag = makeAtomic(plusNfa, L_CHAR, '+');
    nfaViews[2]->buildFromNFA(plusFrag, plusNfa, "+");
    
    FullNFA minusNfa;
    minusNfa.start = minusNfa.newState();
    auto minusFrag = makeAtomic(minusNfa, L_CHAR, '-');
    nfaViews[3]->buildFromNFA(minusFrag, minusNfa, "-");
    
    FullNFA starNfa;
    starNfa.start = starNfa.newState();
    auto starFragOp = makeAtomic(starNfa, L_CHAR, '*');
    nfaViews[4]->buildFromNFA(starFragOp, starNfa, "*");
    
    FullNFA slashNfa;
    slashNfa.start = slashNfa.newState();
    auto slashFrag = makeAtomic(slashNfa, L_CHAR, '/');
    nfaViews[5]->buildFromNFA(slashFrag, slashNfa, "/");
    
    FullNFA lparenNfa;
    lparenNfa.start = lparenNfa.newState();
    auto lparenFrag = makeAtomic(lparenNfa, L_CHAR, '(');
    nfaViews[6]->buildFromNFA(lparenFrag, lparenNfa, "(");
    
    FullNFA rparenNfa;
    rparenNfa.start = rparenNfa.newState();
    auto rparenFrag = makeAtomic(rparenNfa, L_CHAR, ')');
    nfaViews[7]->buildFromNFA(rparenFrag, rparenNfa, ")");
    
    // Whitespace
    FullNFA wsNfa;
    wsNfa.start = wsNfa.newState();
    auto space = makeAtomic(wsNfa, L_CHAR, ' ');
    auto tab = makeAtomic(wsNfa, L_CHAR, '\t');
    auto wsUnion = unionFrag(wsNfa, space, tab);
    nfaViews[8]->buildFromNFA(starFrag(wsNfa, wsUnion), wsNfa, "( | \\t)*");
    
    trace->append("✅ Individual NFAs constructed using Thompson's Construction!");
}

void MainWindow::onLex() {
    tokensBox->clear();
    trace->clear();
    cur = input->text().toStdString();
    tokens = tokenize(dfa, cur);
    
    if(tokens.empty()) {
        trace->append("❌ Lexical error.");
        dfaInfo->setText("<b style='color:red;'>❌ Tokenization Failed</b><br>Invalid character detected.");
        return;
    }
    
    for(size_t i = 0; i < tokens.size(); i++) {
        if(tokens[i].id == 0)
            tokensBox->addItem(QString("%1: EOF").arg(i));
        else
            tokensBox->addItem(QString("%1: %2 '%3'")
                .arg(i)
                .arg(QString::fromStdString(tokenNames[tokens[i].id]))
                .arg(QString::fromStdString(tokens[i].lexeme)));
    }
    
    // Display DFA construction info
    int totalDFAStates = dfa.size();
    int acceptStates = 0;
    for(const auto &state : dfa) {
        if(state.accept) acceptStates++;
    }
    
    dfaInfo->setText(QString(
        "<b style='color:green;'>✅ DFA Construction Complete</b><br>"
        "<b>Subset Construction Result:</b><br>"
        "• Total DFA States: <b>%1</b><br>"
        "• Accept States: <b>%2</b><br>"
        "• Start State: <b>q0</b> (single circle)<br>"
        "• Tokens Found: <b>%3</b><br>"
        "<i>💡 The DFA was built by combining all token NFAs and applying subset construction.</i>"
    ).arg(totalDFAStates).arg(acceptStates).arg(tokens.size() - 1)); // -1 for EOF
    
    trace->append("✅ Lexing complete.");
    trace->append(QString("📊 DFA has %1 states (%2 accept states)").arg(totalDFAStates).arg(acceptStates));
    updateInputDisplay();
    resetDFA();
}

void MainWindow::onParse() {
    trace->clear();
    parseTrace->clear();
    
    if(tokens.empty()) {
        trace->append("⚠ Please lex first.");
        parseTrace->append("⚠ Please tokenize first (Tab 2).");
        return;
    }
    
    parser.reset();
    pdaAnimationSteps.clear();
    
    parseTrace->append("🔨 Analyzing expression and building PDA animation...");
    parseTrace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Start state
    pdaAnimationSteps.push_back({"Start", "Initialize parser", "Push $ and E", "q_start"});
    
    int step = 1;
    while(!parser.isDone()) {
        auto stackBefore = parser.getStack();
        int posBefore = parser.getCurrentPosition();
        
        bool success = parser.stepParse(tokens);
        
        if(!success) {
            pdaAnimationSteps.push_back({"Error", "❌ Parse Error", "Invalid syntax", "q_start"});
            parseTrace->append("<b style='color:red;'>❌ REJECTED</b>");
            trace->append("❌ Parse Rejected");
            
            pdaAnimStep = 0;
            pdaTimer->start(1000);
            return;
        }
        
        auto stackAfter = parser.getStack();
        int posAfter = parser.getCurrentPosition();
        
        // Determine which state we're in based on stack top
        std::string stateId = "q_start";
        QString stateName = "Processing";
        QString action = "Processing";
        QString stackOp = "...";
        
        if(!stackBefore.empty()) {
            std::string top = stackBefore.back();
            
            // Map stack symbol to state
            if(top == "E") {
                stateId = "q_E";
                stateName = "E (Expression)";
                action = "Expanding E → TE'";
                stackOp = "Pop E, Push TE'";
            } else if(top == "E'") {
                stateId = "q_Ep";
                stateName = "E' (Expr continuation)";
                if(stackAfter.size() > stackBefore.size()) {
                    action = "E' → +TE' or -TE'";
                    stackOp = "Push operator and T";
                } else {
                    action = "E' → ε (done with expression)";
                    stackOp = "Pop E'";
                }
            } else if(top == "T") {
                stateId = "q_T";
                stateName = "T (Term)";
                action = "Expanding T → FT'";
                stackOp = "Pop T, Push FT'";
            } else if(top == "T'") {
                stateId = "q_Tp";
                stateName = "T' (Term continuation)";
                if(stackAfter.size() > stackBefore.size()) {
                    action = "T' → *FT' or /FT'";
                    stackOp = "Push operator and F";
                } else {
                    action = "T' → ε (done with term)";
                    stackOp = "Pop T'";
                }
            } else if(top == "F") {
                stateId = "q_F";
                stateName = "F (Factor)";
                action = "Expanding F";
                stackOp = "Pop F, determine production";
            } else if(top == "ID") {
                stateId = "q_ID";
                stateName = "ID (Identifier)";
                action = QString("Match identifier: %1").arg(QString::fromStdString(tokens[posBefore].lexeme));
                stackOp = "Pop ID, consume token";
            } else if(top == "NUMBER") {
                stateId = "q_NUM";
                stateName = "NUMBER";
                action = QString("Match number: %1").arg(QString::fromStdString(tokens[posBefore].lexeme));
                stackOp = "Pop NUMBER, consume token";
            } else if(top == "+" || top == "-" || top == "*" || top == "/" || top == "(" || top == ")") {
                stateId = "q_OP";
                stateName = "Operator";
                action = QString("Match operator: %1").arg(QString::fromStdString(top));
                stackOp = QString("Pop %1, consume token").arg(QString::fromStdString(top));
            }
        }
        
        pdaAnimationSteps.push_back({stateName, action, stackOp, stateId});
        
        updateStack();
        step++;
    }
    
    // Accept state
    pdaAnimationSteps.push_back({"Accept", "✅ Input fully parsed", "Stack empty, $ matched", "q_accept"});
    
    parseTrace->append(QString("📊 Generated %1 animation steps").arg(pdaAnimationSteps.size()));
    parseTrace->append("▶ Starting PDA state-by-state animation...");
    parseTrace->append("");
    trace->append("✅ Parse Accepted");
    
    // Start animation
    pdaAnimStep = 0;
    updateParseInputDisplay();
    pdaTimer->start(1200);  // 1.2 seconds per step
}

void MainWindow::onAnimateDFA() {
    if(tokens.empty()) {
        trace->append("⚠ Please lex first.");
        return;
    }
    
    // FIRST: Check if parse will succeed
    Parser testParser;
    bool parseWillSucceed = testParser.parseAll(tokens);
    
    if(!parseWillSucceed) {
        trace->append("❌ Cannot animate: Expression is invalid (parse rejected)");
        trace->append("💡 DFA animation only runs for valid expressions");
        return;
    }
    
    // Parse succeeded, proceed with animation
    animationSteps.clear();
    currentTokenIndex = 0;
    
    for(const auto &token : tokens) {
        if(token.id == 0) break;
        
        std::string lexeme = token.lexeme;
        int state = 0;
        
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
        
        if(&token != &tokens.back() - 1) {
            animationSteps.push_back({state, 0, '\0', -1, "RESET"});
        }
    }
    
    animationStep = 0;
    trace->append("✅ Expression is valid!");
    trace->append("▶ Starting DFA animation for valid expression...");
    trace->append(QString("Total steps: %1").arg(animationSteps.size()));
    timer->start(600);
}

void MainWindow::onStopDFA() {
    timer->stop();
    trace->append("⏸ Animation stopped.");
}

void MainWindow::onResetDFA() {
    timer->stop();
    resetDFA();
    trace->append("🔄 DFA reset.");
    updateInputDisplay();
}

void MainWindow::dfaStepTimer() {
    if(animationStep >= (int)animationSteps.size()) {
        trace->append("✅ Token recognition complete!");
        timer->stop();
        return;
    }

    auto &step = animationSteps[animationStep];
    
    if(step.ch == '\0') {
        view->highlightState(0);
        trace->append("🔄 DFA Reset → q0 (ready for next token)");
        dfaState = 0;
        dfaPos = 0;
    } else {
        dfaState = step.toState;
        dfaPos = step.inputPos;
        view->highlightState(dfaState);
        updateInputDisplay();

        QString charDisplay = (step.ch == ' ') ? "␣" : (step.ch == '\t') ? "⇥" : QString(QChar(step.ch));
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

void MainWindow::onStepParse() {
    if(tokens.empty()) {
        trace->append("⚠ Please lex first.");
        parseTrace->append("⚠ Please tokenize first (Tab 2).");
        return;
    }
    if(parser.isDone()) return;
    
    auto stackBefore = parser.getStack();
    
    if(parser.stepParse(tokens)) {
        updateStack();
        updatePDAState();
        updateParseInputDisplay();
        
        auto stackAfter = parser.getStack();
        
        if(stackBefore != stackAfter) {
            parseTrace->append(QString("✓ Stack: %1 → %2")
                .arg(stackBefore.empty() ? "$" : QString::fromStdString(stackBefore.back()))
                .arg(stackAfter.empty() ? "$" : QString::fromStdString(stackAfter.back())));
        }
        
        if(parser.isDone()) {
            trace->append("✅ Parse accepted.");
            parseTrace->append("<b style='color:green;'>✅ Parse Complete - ACCEPTED</b>");
        }
    } else {
        parseTrace->append("<b style='color:red;'>❌ Parse error</b>");
    }
}

void MainWindow::onResetParse() {
    resetParse();
    trace->append("🔄 Parser reset.");
    parseTrace->clear();
    parseTrace->append("🔄 Parser reset - Ready to parse");
    updateParseInputDisplay();
}

void MainWindow::onClearOutput() {
    trace->clear();
    tokensBox->clear();
    stackBox->clear();
}

void MainWindow::resetDFA() {
    dfaState = 0;
    dfaPos = 0;
    view->highlightState(dfaState);
}

void MainWindow::updatePDAState() {
    if(!pdaQ0Circle || !pdaQ1Circle) return;
    
    // Reset all to normal colors
    // Note: We only have q2 and q3 stored, so this is simplified
    
    if(parser.isDone() && parser.getStack().empty()) {
        // Accept state (q3)
        pdaPhase = 3;
        pdaQ0Circle->setBrush(QBrush(QColor(173, 216, 230))); // Normal
        pdaQ0Circle->setPen(QPen(Qt::black, 2));
        pdaQ1Circle->setBrush(QBrush(QColor(255, 215, 0))); // Gold
        pdaQ1Circle->setPen(QPen(Qt::red, 3));
    } else if(!parser.getStack().empty()) {
        // Parsing state (q2)
        pdaPhase = 2;
        pdaQ0Circle->setBrush(QBrush(QColor(255, 255, 100))); // Yellow
        pdaQ0Circle->setPen(QPen(Qt::red, 3));
        pdaQ1Circle->setBrush(QBrush(QColor(144, 238, 144))); // Normal
        pdaQ1Circle->setPen(QPen(Qt::black, 2));
    }
}

void MainWindow::resetParse() {
    parser.reset();
    pdaState = 0;
    pdaPhase = 0;
    updateStack();
    updatePDAState();
}

void MainWindow::updateStack() {
    stackBox->clear();
    auto stack = parser.getStack();
    for(int i = stack.size() - 1; i >= 0; i--) {
        stackBox->addItem(QString::fromStdString(stack[i]));
    }
}

void MainWindow::updateInputDisplay() {
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
            html += QString("<span style='color: #888;'>%1</span>").arg(charStr);
        } else if(i == dfaPos && animationStep < (int)animationSteps.size()) {
            html += QString("<span style='background-color: yellow; font-weight: bold; padding: 2px;'>%1</span>").arg(charStr);
        } else {
            html += charStr;
        }
    }
    
    html += QString(" | <b>State:</b> q%1").arg(dfaState);
    
    if(animationStep < (int)animationSteps.size() && animationSteps[animationStep].ch != '\0') {
        html += QString(" | <b>Recognizing:</b> %1").arg(animationSteps[animationStep].tokenName);
    }
    
    html += "</body></html>";
    inputDisplay->setText(html);
}

void MainWindow::drawPDA() {
    QGraphicsScene *scene = pdaView->scene();
    scene->clear();
    
    // Clear layout - no overlaps
    double radius = 26;
    QFont stateFont;
    stateFont.setPointSize(7);
    stateFont.setBold(true);
    
    // Grid layout with more space
    double startX = 70;
    double spacing = 110;
    
    double col0 = startX;
    double col1 = col0 + spacing;
    double col2 = col1 + spacing;
    double col3 = col2 + spacing;
    double col4 = col3 + spacing;
    double col5 = col4 + spacing;
    
    double centerY = 100;
    double topY = 40;
    double bottomY = 160;
    
    // State storage for animation
    allPDAStates.clear();
    
    // Helper to draw state
    auto drawState = [&](double x, double y, const QString &id, const QString &label, bool isAccept, const QColor &color = QColor(220, 230, 255)) {
        QGraphicsEllipseItem *circle = scene->addEllipse(
            x - radius, y - radius, 2*radius, 2*radius,
            QPen(Qt::black, 2), QBrush(color)
        );
        
        if(isAccept) {
            scene->addEllipse(x - radius + 4, y - radius + 4, 
                            2*radius - 8, 2*radius - 8, QPen(Qt::black, 2));
        }
        
        QGraphicsTextItem *text = scene->addText(label);
        text->setFont(stateFont);
        QRectF bounds = text->boundingRect();
        text->setPos(x - bounds.width()/2, y - bounds.height()/2);
        
        allPDAStates[id.toStdString()] = circle;
    };
    
    // Helper to draw arrow
    auto drawArrow = [&](double x1, double y1, double x2, double y2, const QString &label) {
        double angle = std::atan2(y2 - y1, x2 - x1);
        double startX = x1 + radius * std::cos(angle);
        double startY = y1 + radius * std::sin(angle);
        double endX = x2 - radius * std::cos(angle);
        double endY = y2 - radius * std::sin(angle);
        
        QLineF line(startX, startY, endX, endY);
        scene->addLine(line, QPen(QColor(100, 100, 100), 1.5));
        
        // Arrowhead
        QPointF p1 = line.p2() - QPointF(9*std::cos(angle+M_PI/6), 9*std::sin(angle+M_PI/6));
        QPointF p2 = line.p2() - QPointF(9*std::cos(angle-M_PI/6), 9*std::sin(angle-M_PI/6));
        QPolygonF arrow;
        arrow << line.p2() << p1 << p2;
        scene->addPolygon(arrow, QPen(QColor(100, 100, 100)), QBrush(QColor(100, 100, 100)));
        
        // Label
        if(!label.isEmpty()) {
            QGraphicsTextItem *lbl = scene->addText(label);
            QFont lblFont;
            lblFont.setPointSize(6);
            lbl->setFont(lblFont);
            lbl->setDefaultTextColor(Qt::blue);
            QRectF lblBounds = lbl->boundingRect();
            lbl->setPos((startX+endX)/2 - lblBounds.width()/2, (startY+endY)/2 - 20);
        }
    };
    
    // START arrow
    QLineF startArrow(10, centerY, col0 - radius - 5, centerY);
    scene->addLine(startArrow, QPen(Qt::darkBlue, 3));
    QPolygonF startHead;
    startHead << startArrow.p2() << (startArrow.p2()-QPointF(10,5)) << (startArrow.p2()-QPointF(10,-5));
    scene->addPolygon(startHead, QPen(Qt::darkBlue), QBrush(Qt::darkBlue));
    
    QGraphicsTextItem *startTxt = scene->addText("START");
    QFont startFont;
    startFont.setPointSize(7);
    startFont.setBold(true);
    startTxt->setFont(startFont);
    startTxt->setDefaultTextColor(Qt::darkBlue);
    startTxt->setPos(5, centerY - 30);
    
    // Row 1: Main grammar flow (top)
    drawState(col0, centerY, "q_start", "Start", false);
    drawState(col1, topY, "q_E", "E", false, QColor(200, 220, 255));
    drawState(col2, topY, "q_T", "T", false, QColor(220, 200, 255));
    drawState(col3, topY, "q_F", "F", false, QColor(255, 220, 180));
    
    // Row 2: Terminals (center)
    drawState(col3, bottomY, "q_ID", "ID", false, QColor(200, 255, 200));
    drawState(col4, topY, "q_NUM", "NUM", false, QColor(200, 255, 200));
    drawState(col4, centerY, "q_OP", "Op", false, QColor(200, 255, 200));
    
    // Row 3: Continuations (bottom)
    drawState(col1, bottomY, "q_Ep", "E'", false, QColor(200, 220, 255));
    drawState(col2, bottomY, "q_Tp", "T'", false, QColor(220, 200, 255));
    
    // Accept (end)
    drawState(col5, bottomY, "q_accept", "Accept", true, QColor(144, 238, 144));
    
    // Store references for highlighting
    pdaQ0Circle = allPDAStates["q_start"];
    pdaQ1Circle = allPDAStates["q_accept"];
    
    // Draw connections (grammar flow)
    drawArrow(col0, centerY, col1, topY, "E");
    drawArrow(col1, topY, col2, topY, "T");
    drawArrow(col2, topY, col3, topY, "F");
    drawArrow(col3, topY, col3, bottomY, "ID");
    drawArrow(col3, topY, col4, topY, "NUM");
    drawArrow(col3, topY, col4, centerY, "Op");
    drawArrow(col3, bottomY, col5, bottomY, "");
    drawArrow(col4, topY, col5, bottomY, "");
    drawArrow(col4, centerY, col5, bottomY, "");
    
    // E' and T' paths
    drawArrow(col1, topY, col1, bottomY, "E'");
    drawArrow(col2, topY, col2, bottomY, "T'");
    drawArrow(col1, bottomY, col2, topY, "+T|-T");
    drawArrow(col2, bottomY, col3, topY, "*F|/F");
    
    // Legend
    QGraphicsTextItem *legend = scene->addText(
        "Blue = Non-terminals (E,T,F) | Green = Terminals (ID,NUM,Op) | Yellow = Active | Gold = Accept"
    );
    QFont legendFont;
    legendFont.setPointSize(6);
    legend->setFont(legendFont);
    legend->setDefaultTextColor(QColor(80, 80, 80));
    legend->setPos(10, bottomY + 50);
    
    scene->setSceneRect(scene->itemsBoundingRect().adjusted(-10, -10, 10, 10));
}

void MainWindow::pdaStepTimer() {
    if(pdaAnimStep >= (int)pdaAnimationSteps.size()) {
        pdaTimer->stop();
        parseTrace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        parseTrace->append("✅ PDA animation complete!");
        return;
    }
    
    auto &step = pdaAnimationSteps[pdaAnimStep];
    
    // Reset all states to normal
    for(auto &pair : allPDAStates) {
        QGraphicsEllipseItem *circle = pair.second;
        
        // Determine default color based on state type
        QString stateKey = QString::fromStdString(pair.first);
        QColor defaultColor;
        
        if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
            defaultColor = QColor(200, 220, 255); // Blue for E
        } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
            defaultColor = QColor(220, 200, 255); // Purple for T
        } else if(stateKey.contains("_F")) {
            defaultColor = QColor(255, 220, 180); // Orange for F
        } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
            defaultColor = QColor(200, 255, 200); // Green for terminals
        } else if(stateKey.contains("accept")) {
            defaultColor = QColor(144, 238, 144); // Green for accept
        } else {
            defaultColor = QColor(220, 230, 255); // Default blue
        }
        
        circle->setBrush(QBrush(defaultColor));
        circle->setPen(QPen(Qt::black, 2));
    }
    
    // Highlight current state
    if(allPDAStates.count(step.stateId)) {
        QGraphicsEllipseItem *activeCircle = allPDAStates[step.stateId];
        activeCircle->setBrush(QBrush(QColor(255, 255, 100))); // Yellow
        activeCircle->setPen(QPen(Qt::red, 3));
    }
    
    // Show action in trace
    parseTrace->append(QString("<b>Step %1:</b> In state <b>%2</b>")
        .arg(pdaAnimStep + 1)
        .arg(step.stateName));
    parseTrace->append(QString("  → %1").arg(step.action));
    parseTrace->append(QString("  → Stack: %1").arg(step.stackOp));
    parseTrace->append("");
    
    pdaAnimStep++;
}