#include "mainwindow.h"
#include "core/thompson.h"
#include "core/subset.h"
#include "lexer/tokenizer.h"
#include "parser/grammar.h"
#include "validator/validator.h"
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
    // TAB 1: Project Overview + Thompson's Construction
    // ============================================
    QWidget *tab1 = new QWidget();
    QVBoxLayout *tab1Layout = new QVBoxLayout(tab1);

    // Project Overview Section
    QLabel *projectTitle = new QLabel(
        "<h1 style='color:#2C5F2D;'>🎓 Compiler Front-End Visualizer</h1>"
    );
    projectTitle->setTextFormat(Qt::RichText);
    projectTitle->setAlignment(Qt::AlignCenter);
    tab1Layout->addWidget(projectTitle);

    // QTextEdit *overviewText = new QTextEdit();
    // overviewText->setReadOnly(true);
    // overviewText->setMaximumHeight(150);
    // overviewText->setHtml(
    //     "<h3>📚 Project Overview</h3>"
    //     "<p>This application demonstrates the <b>formal language hierarchy</b> in compiler design:</p>"
    //     "<ul>"
    //     "<li><b>Regular Languages (Type 3):</b> Handled by Finite Automata (DFA/NFA) for <b>Lexical Analysis</b></li>"
    //     "<li><b>Context-Free Languages (Type 2):</b> Handled by Pushdown Automata (PDA) for <b>Syntactic Analysis</b></li>"
    //     "</ul>"
    //     "<p><b>Supported Language:</b> Arithmetic expressions with identifiers, numbers, and operators (+, -, *, /, parentheses)</p>"
    // );
    // tab1Layout->addWidget(overviewText);

    // // Thompson's Construction Section
    // QLabel *thompsonTitle = new QLabel("<h2 style='color:#2C5F2D;'>Thompson's Construction Algorithm</h2>");
    // thompsonTitle->setTextFormat(Qt::RichText);
    // tab1Layout->addWidget(thompsonTitle);

    // QTextEdit *thompsonExplanation = new QTextEdit();
    // thompsonExplanation->setReadOnly(true);
    // thompsonExplanation->setMaximumHeight(120);  // Reduced from 150
    // thompsonExplanation->setStyleSheet("QTextEdit { background-color: #F0F8F0; }");
    // thompsonExplanation->setHtml(
    //     "<b>Thompson's Construction</b> builds NFAs compositionally using these building blocks:<br><br>"
    //     "<b>1. Atomic:</b> Single character → 2-state NFA<br>"
    //     "<b>2. Concatenation (ab):</b> Connect NFAs with ε-transition<br>"
    //     "<b>3. Union (a|b):</b> Create new start/accept states with ε-transitions<br>"
    //     "<b>4. Kleene Star (a*):</b> Add ε-loops for zero-or-more repetition<br><br>"
    //     "<i>Click below to see the complete NFA with consistent state numbering.</i>"
    // );
    // tab1Layout->addWidget(thompsonExplanation);

    auto *buildNfaBtn = new QPushButton("Show Thompson's NFA");
    buildNfaBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 10px; }");
    tab1Layout->addWidget(buildNfaBtn);

    // UPDATED: Single large NFA view instead of 9 separate views
    QScrollArea *nfaScrollArea = new QScrollArea();
    QWidget *nfaContainer = new QWidget();
    QVBoxLayout *nfaContainerLayout = new QVBoxLayout(nfaContainer);

    // Create ONE combined NFA view
    NFAView *combinedNFAView = new NFAView();
    combinedNFAView->setMaximumHeight(500);  // Large enough for complex diagram
    nfaViews.push_back(combinedNFAView);

    QGroupBox *groupBox = new QGroupBox("NFA: All Token Types with Consistent State Numbering");
    QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);

    // Explanation label
    QLabel *explanation = new QLabel();
    explanation->setWordWrap(true);
    explanation->setStyleSheet("QLabel { color: #555; font-size: 8pt; padding: 8px; background-color: #FFFEF0; border-left: 4px solid #4CAF50; }");
    explanation->setText(
        "📊 <b>This diagram shows the COMPLETE NFA</b> built using Thompson's Construction. <br>"
        "<b>All token types (ID, NUMBER, PLUS, MINUS, STAR, SLASH, LPAREN, RPAREN, WHITESPACE) are into a single automaton with <b>consistent state numbering</b> (q0, q1, q2, ...).<br>"
        "<b>🔹 The super-start state q0</b> connects to each token's sub-NFA via <b>ε-transitions.<br>"
        "<b>🔹 Each <b>accept state</b> is labeled with its token type (e.g., [ID], [NUMBER]).<br>"
        "<b>🔹 This NFA is then converted to DFA using <b>Subset Construction</b> (see Tab 2).<br>"
    );

    groupLayout->addWidget(explanation);
    groupLayout->addWidget(combinedNFAView);

    nfaContainerLayout->addWidget(groupBox);

    // Add legend/token reference
    QTextEdit *tokenReference = new QTextEdit();
    tokenReference->setReadOnly(true);
    tokenReference->setMaximumHeight(120);
    tokenReference->setStyleSheet("QTextEdit { background-color: #F5F5DC; font-family: 'Courier New'; font-size: 8pt; }");
    tokenReference->setHtml(
        "<b>Token Types in NFA:</b><br>"
        "<table style='width:100%; font-size: 8pt;'>"
        "<tr><td><b>ID:</b></td><td>[a-zA-Z][a-zA-Z0-9_]*</td><td style='color:#666;'>(identifiers: a, foo, var_1)</td></tr>"
        "<tr><td><b>NUMBER:</b></td><td>[0-9]+(\\.[0-9]+)?</td><td style='color:#666;'>(integers & decimals: 42, 3.14)</td></tr>"
        "<tr><td><b>PLUS:</b></td><td>+</td><td style='color:#666;'>(addition operator)</td></tr>"
        "<tr><td><b>MINUS:</b></td><td>-</td><td style='color:#666;'>(subtraction/negation)</td></tr>"
        "<tr><td><b>STAR:</b></td><td>*</td><td style='color:#666;'>(multiplication operator)</td></tr>"
        "<tr><td><b>SLASH:</b></td><td>/</td><td style='color:#666;'>(division operator)</td></tr>"
        "<tr><td><b>LPAREN:</b></td><td>(</td><td style='color:#666;'>(left parenthesis)</td></tr>"
        "<tr><td><b>RPAREN:</b></td><td>)</td><td style='color:#666;'>(right parenthesis)</td></tr>"
        "<tr><td><b>WHITESPACE:</b></td><td>( | \\t)+</td><td style='color:#666;'>(spaces and tabs)</td></tr>"
        "</table>"
    );
    nfaContainerLayout->addWidget(tokenReference);

    nfaScrollArea->setWidget(nfaContainer);
    nfaScrollArea->setWidgetResizable(true);
    tab1Layout->addWidget(nfaScrollArea);

    tabWidget->addTab(tab1, "1. Thompson's NFA");
    
    // ============================================
    // TAB 2: DFA Conversion (Subset Construction)
    // ============================================
    QWidget *tab2 = new QWidget();
    QVBoxLayout *tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setSpacing(8); // Tighter spacing
    tab2Layout->setContentsMargins(10, 10, 10, 10);

    QLabel *tab2Title = new QLabel("<h2 style='color:#1E5F8C; margin:0;'>DFA & Tokenization</h2>");
    tab2Title->setTextFormat(Qt::RichText);
    tab2Layout->addWidget(tab2Title);

    // Compact explanation (collapsible would be better, but simplified for now)
    QTextEdit *subsetExplanation = new QTextEdit();
    subsetExplanation->setReadOnly(true);
    subsetExplanation->setMaximumHeight(40); // Reduced from 120
    subsetExplanation->setStyleSheet("QTextEdit { background-color: #E8F4F8; font-size: 8pt; padding: 5px; }");
    subsetExplanation->setHtml(
        "<b>Subset Construction:</b> Converts NFA → DFA by treating sets of NFA states as single DFA states. "
        "The DFA below is simplified (5 states) for educational clarity."
    );
    tab2Layout->addWidget(subsetExplanation);

    // Buttons in compact row
    auto *lexBtn = new QPushButton("Tokenize");
    auto *animBtn = new QPushButton("Animate DFA");
    auto *stopBtn = new QPushButton("Stop");
    auto *resetBtn = new QPushButton("Reset");
    auto *clearBtn = new QPushButton("Clear");

    lexBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    animBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");

    auto *tab2Btns = new QHBoxLayout();
    tab2Btns->setSpacing(5);
    tab2Btns->addWidget(lexBtn);
    tab2Btns->addWidget(animBtn);
    tab2Btns->addWidget(stopBtn);
    tab2Btns->addWidget(resetBtn);
    tab2Btns->addWidget(clearBtn);
    tab2Btns->addStretch();
    tab2Layout->addLayout(tab2Btns);

    // Input display
    inputDisplay = new QLabel();
    inputDisplay->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; font-family: monospace; font-size: 10pt; }");
    inputDisplay->setMaximumHeight(30);
    tab2Layout->addWidget(inputDisplay);

    // DFA View - Give it more space!
    view = new AutomataView();
    view->setMinimumHeight(400); // Increased
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tab2Layout->addWidget(view, 1); // Stretch factor 1

    // DFA info (compact)
    dfaInfo = new QLabel();
    dfaInfo->setTextFormat(Qt::RichText);
    dfaInfo->setWordWrap(true);
    dfaInfo->setMinimumHeight(50); // Reduced
    dfaInfo->setStyleSheet("QLabel { background-color: #E8F4F8; padding: 8px; border: 1px solid #1E5F8C; border-radius: 3px; font-size: 8pt; }");
    dfaInfo->setText(
        "<b>📊 Simplified Educational DFA:</b> "
        "5 states (q0-q4)"
    );
    tab2Layout->addWidget(dfaInfo);

    // Tokens and trace side by side (compact)
    auto *tab2Bottom = new QHBoxLayout();
    tab2Bottom->setSpacing(10);

    auto *tokensLayout = new QVBoxLayout();
    tokensLayout->addWidget(new QLabel("<b>Tokens:</b>"));
    tokensBox = new QListWidget();
    tokensBox->setMaximumHeight(150); // Reduced from 150
    tokensBox->setStyleSheet("QListWidget { font-size: 9pt; }");
    tokensLayout->addWidget(tokensBox);
    tab2Bottom->addLayout(tokensLayout);

    auto *traceLayout = new QVBoxLayout();
    traceLayout->addWidget(new QLabel("<b>Output Trace:</b>"));
    trace = new QTextEdit();
    trace->setReadOnly(true);
    trace->setMaximumHeight(150); // Reduced from 150
    trace->setStyleSheet("QTextEdit { font-size: 9pt; }");
    traceLayout->addWidget(trace);
    tab2Bottom->addLayout(traceLayout);

    tab2Layout->addLayout(tab2Bottom);

    tabWidget->addTab(tab2, "2. DFA & Tokenization");
    
    
    // ============================================
    // TAB 3: PDA Parser (Syntactic Analysis)
    // ============================================
    QWidget *tab3 = new QWidget();
    QVBoxLayout *tab3Layout = new QVBoxLayout(tab3);

    QLabel *tab3Title = new QLabel("<h2 style='color:#8B4513;'>Tab 3: PDA Parser (Syntactic Analysis)</h2>");
    tab3Title->setTextFormat(Qt::RichText);
    tab3Layout->addWidget(tab3Title);

    // BUTTONS ROW
    auto *parseBtn = new QPushButton("Parse Expression");
    auto *animateParseBtn = new QPushButton("Animate Parse");
    auto *stepParseBtn = new QPushButton("Step Parse");
    auto *resetParseBtn = new QPushButton("Reset Parser");

    auto *tab3Btns = new QHBoxLayout();
    tab3Btns->addWidget(parseBtn);
    tab3Btns->addWidget(animateParseBtn);
    tab3Btns->addWidget(stepParseBtn);
    tab3Btns->addWidget(resetParseBtn);
    tab3Layout->addLayout(tab3Btns);

    // INPUT DISPLAY for parser
    parseInputDisplay = new QLabel();
    parseInputDisplay->setStyleSheet("QLabel { background-color: #FFF8DC; padding: 5px; font-family: monospace; font-size: 11pt; border: 1px solid #8B4513; }");
    parseInputDisplay->setAlignment(Qt::AlignLeft);
    tab3Layout->addWidget(parseInputDisplay);

    // PDA Visualization
    pdaView = new QGraphicsView();
    pdaView->setScene(new QGraphicsScene(pdaView));
    pdaView->setRenderHint(QPainter::Antialiasing);
    pdaView->setMaximumHeight(250);
    pdaView->setStyleSheet("QGraphicsView { background-color: #FFF8DC; border: 2px solid #8B4513; }");
    tab3Layout->addWidget(pdaView);

    // Middle section: Stack and Grammar side by side
    auto *tab3Middle = new QHBoxLayout();

    // LEFT: Stack display
    auto *stackLayout = new QVBoxLayout();
    stackLayout->addWidget(new QLabel("<b>PDA Stack:</b>"));
    stackBox = new QListWidget();
    stackBox->setMaximumHeight(200);
    stackBox->setStyleSheet("QListWidget { background-color: #FFFACD; font-family: 'Courier New'; font-size: 10pt; }");
    stackLayout->addWidget(stackBox);
    tab3Middle->addLayout(stackLayout);

    // RIGHT: Grammar and Parsing Table
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

    // Fill table with productions
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

    // CFG productions below table
    grammarDisplay = new QTextEdit();
    grammarDisplay->setReadOnly(true);
    grammarDisplay->setMaximumHeight(100);
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

    // OUTPUT TRACE (like Tab 2)
    outputParseBox = new QTextEdit();
    outputParseBox->setReadOnly(true);
    outputParseBox->setMaximumHeight(120);
    outputParseBox->setPlaceholderText("Output trace will appear here...");
    outputParseBox->setStyleSheet("QTextEdit { background-color: #F0FFF0; font-family: 'Courier New'; font-size: 9pt; }");
    tab3Layout->addWidget(new QLabel("<b>Output Trace:</b>"));
    tab3Layout->addWidget(outputParseBox);

    // PARSE TRACE (detailed steps)
    parseTrace = new QTextEdit();
    parseTrace->setReadOnly(true);
    parseTrace->setMaximumHeight(150);
    parseTrace->setPlaceholderText("Detailed parse trace will appear here...");
    parseTrace->setStyleSheet("QTextEdit { background-color: #FFFAF0; font-family: 'Courier New'; font-size: 9pt; }");
    tab3Layout->addWidget(new QLabel("<b>Detailed Parse Trace:</b>"));
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
    dfa = subsetConstruct(nfa);  // Keep real DFA for tokenization
    buildSimplifiedDFA();  // Build simplified DFA for visualization
    view->buildFromDFA(simplifiedDFA);  // Display simplified version
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
    connect(animateParseBtn, &QPushButton::clicked, this, &MainWindow::onAnimateParse);
    
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
            // Already consumed - gray and strikethrough
            html += QString("<span style='color: #888; text-decoration: line-through;'>%1 </span>").arg(tokenStr);
        } else if((int)i == currentPos) {
            // Current token being processed - yellow highlight
            html += QString("<span style='background-color: yellow; font-weight: bold; padding: 2px;'>%1</span> ").arg(tokenStr);
        } else {
            // Not yet consumed - normal
            html += tokenStr + " ";
        }
    }
    
    auto stack = parser.getStack();
    html += QString(" | <b>Stack Top:</b> %1").arg(
        stack.empty() ? "(empty)" : QString::fromStdString(stack.back())
    );
    
    html += "</body></html>";
    parseInputDisplay->setText(html);
}

void MainWindow::onBuildNFA() {
    // Instead of building individual NFAs, show the COMBINED NFA with consistent numbering
    
    // Clear all NFA views first
    for(auto *view : nfaViews) {
        view->clear();
    }
    
    // Build the complete combined NFA (same as used for tokenization)
    FullNFA combinedNFA = buildCombinedNFA();
    
    trace->append("✅ NFA constructed using Thompson's Construction!");
    trace->append(QString("📊 Total NFA states: %1").arg(combinedNFA.states.size()));
    trace->append(QString("🎯 Start state: q%1").arg(combinedNFA.start));
    trace->append(QString("✓ Accept states: %1").arg(combinedNFA.acceptToken.size()));
    
    // For Tab 1, show a SINGLE combined NFA diagram instead of 9 separate ones
    // We'll use the first nfaView as a larger combined view
    
    if(!nfaViews.empty()) {
        // Build combined view showing all token paths
        nfaViews[0]->buildCombinedNFA(combinedNFA);
        
        // Hide other views or show summary
        for(size_t i = 1; i < nfaViews.size(); i++) {
            nfaViews[i]->hide();
        }
    }
    
    // Update explanation
    trace->append("");
    trace->append("ℹ️ The NFA shows how Thompson's Construction:");
    trace->append("  • Creates a super-start state q0");
    trace->append("  • Connects to each token's NFA via ε-transitions");
    trace->append("  • Maintains consistent state numbering across all tokens");
    trace->append("  • Each accept state is tagged with its token type");
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
    
    int totalDFAStates = dfa.size();
    int acceptStates = 0;
    for(const auto &state : dfa) {
        if(state.accept) acceptStates++;
    }
    
    dfaInfo->setText(QString(
        "<b style='color:green;'>✅ Tokenization Complete</b><br>"
        "<b>Simplified DFA (for animation):</b> 5 states (q0-q4)<br>"
        "• q0: START<br> • q1: ID (identifiers) • q2: NUMBER (integers/decimals) • q3: OPERATOR (+,−,*,/,(,)) • q4: WHITESPACE (spaces/tabs)<br>"
        "<i>💡 Click 'Animate DFA' to see character-by-character processing</i>"
    ).arg(totalDFAStates).arg(acceptStates));
    
    trace->append("✅ Lexing complete.");
    trace->append(QString("📊 Found %1 tokens").arg(tokens.size() - 1));
    updateInputDisplay();
    resetDFA();
}

void MainWindow::onParse() {
    outputParseBox->clear();
    parseTrace->clear();
    
    if(tokens.empty()) {
        outputParseBox->append("⚠ Please tokenize first (Tab 2).");
        return;
    }
    
    // VALIDATION
    outputParseBox->append("🔍 Validating expression structure...");
    ValidationResult validation = ExpressionValidator::validate(tokens);
    
    if(!validation.valid) {
        outputParseBox->append("<b style='color:red;'>❌ VALIDATION FAILED</b>");
        outputParseBox->append(QString("Error: %1").arg(QString::fromStdString(validation.error)));
        return;
    }
    
    outputParseBox->append("✅ Expression structure valid");
    outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // PARSE SILENTLY
    parser.reset();
    outputParseBox->append("🔨 Parsing expression...");
    
    bool success = parser.parseAll(tokens);
    
    if(success) {
        outputParseBox->append("<b style='color:green;'>✅ PARSE ACCEPTED</b>");
        outputParseBox->append("The expression is syntactically valid according to the grammar.");
    } else {
        outputParseBox->append("<b style='color:red;'>❌ PARSE REJECTED</b>");
        outputParseBox->append("Syntax error detected in expression.");
    }
    
    updateStack();
    updateParseInputDisplay();
}

void MainWindow::onAnimateParse() {
    outputParseBox->clear();
    parseTrace->clear();
    
    if(tokens.empty()) {
        outputParseBox->append("⚠ Please tokenize first (Tab 2).");
        return;
    }
    
    // VALIDATION
    ValidationResult validation = ExpressionValidator::validate(tokens);
    if(!validation.valid) {
        outputParseBox->append("<b style='color:red;'>❌ VALIDATION FAILED</b>");
        outputParseBox->append(QString("Error: %1").arg(QString::fromStdString(validation.error)));
        return;
    }
    
    outputParseBox->append("✅ Starting PDA animation...");
    outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Build animation steps using TEMPORARY parser
    Parser tempParser;
    tempParser.reset();
    pdaAnimationSteps.clear();
    
    // Initial step - highlight start state
    pdaAnimationSteps.push_back({"Initialize", "Push $ and E onto stack", "Start state", "q_start"});
    
    int step = 1;
    while(!tempParser.isDone()) {
        auto stackBefore = tempParser.getStack();
        int posBefore = tempParser.getCurrentPosition();
        
        bool success = tempParser.stepParse(tokens);
        
        if(!success) {
            pdaAnimationSteps.push_back({"Error", "Parse failed - syntax error", "Error state reached", "q_error"});
            outputParseBox->append("<b style='color:red;'>❌ PARSE REJECTED</b>");
            break;
        }
        
        auto stackAfter = tempParser.getStack();
        int posAfter = tempParser.getCurrentPosition();
        
        // Determine what happened based on BEFORE state
        std::string stateId = "q_start";
        QString stateName = "Processing";
        QString action = "Processing";
        QString stackOp = "...";
        
        if(!stackBefore.empty()) {
            std::string top = stackBefore.back();
            
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
        step++;
    }
    
    // CRITICAL FIX: Add final accept step that will be displayed
    if(tempParser.isDone() && tempParser.getStack().empty()) {
        pdaAnimationSteps.push_back({"Accept", "✅ Input fully parsed", "Stack empty, $ matched", "q_accept"});
        outputParseBox->append("✅ Parse will accept");
    }
    
    outputParseBox->append(QString("Generated %1 animation steps").arg(pdaAnimationSteps.size()));
    outputParseBox->append("▶ Starting automatic animation...");
    
    // Reset REAL parser for playback (DO NOT call parser.reset() again in timer!)
    parser.reset();
    pdaAnimStep = 0;
    updateStack();
    updateParseInputDisplay();
    
    // Start with highlighting the start state
    for(auto &pair : allPDAStates) {
        QGraphicsEllipseItem *circle = pair.second;
        QString stateKey = QString::fromStdString(pair.first);
        QColor defaultColor;
        
        if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
            defaultColor = QColor(200, 220, 255);
        } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
            defaultColor = QColor(220, 200, 255);
        } else if(stateKey.contains("_F")) {
            defaultColor = QColor(255, 220, 180);
        } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
            defaultColor = QColor(200, 255, 200);
        } else if(stateKey.contains("accept")) {
            defaultColor = QColor(144, 238, 144);
        } else {
            defaultColor = QColor(220, 230, 255);
        }
        
        circle->setBrush(QBrush(defaultColor));
        circle->setPen(QPen(Qt::black, 2));
    }
    
    pdaTimer->start(1000);
}

void MainWindow::onAnimateDFA() {
    if(cur.empty()) {
        trace->append("⚠ Please enter an expression first.");
        return;
    }
    
    if(tokens.empty()) {
        trace->append("⚠ Please tokenize first.");
        return;
    }
    
    trace->clear();
    trace->append("🔍 Validating expression...");
    trace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // STEP 1: Expression Validation
    ValidationResult validation = ExpressionValidator::validate(tokens);
    
    if(!validation.valid) {
        trace->append("<b style='color:red;'>❌ VALIDATION FAILED</b>");
        trace->append(QString("   Error: %1").arg(QString::fromStdString(validation.error)));
        if(validation.errorPosition >= 0 && validation.errorPosition < (int)tokens.size()) {
            trace->append(QString("   At token %1: '%2'")
                .arg(validation.errorPosition)
                .arg(QString::fromStdString(tokens[validation.errorPosition].lexeme)));
        }
        trace->append("");
        trace->append("💡 <b>Common issues:</b>");
        trace->append("   • Adjacent operators: 3++5 → invalid");
        trace->append("   • Ending with operator: a+ → invalid");
        trace->append("   • Unbalanced parentheses: (3+5)) → invalid");
        trace->append("   • Unary without parens: -3 → invalid, use (-3)");
        return;
    }
    
    trace->append("✅ Expression structure valid");
    trace->append("");
    
    // STEP 2: Parser Check
    trace->append("🔍 Checking parser acceptance...");
    Parser testParser;
    bool parseWillSucceed = testParser.parseAll(tokens);
    
    if(!parseWillSucceed) {
        trace->append("<b style='color:red;'>❌ PARSER REJECTED</b>");
        trace->append("   The expression structure is invalid");
        return;
    }
    
    trace->append("✅ Parser accepts expression");
    trace->append("");
    
    // STEP 3: DFA Animation
    animationSteps.clear();
    
    int state = 0;
    std::string inputStr = cur;
    
    trace->append("▶ Starting DFA animation...");
    trace->append(QString("   Input: \"%1\"").arg(QString::fromStdString(inputStr)));
    trace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    bool hasError = false;
    for(size_t i = 0; i < inputStr.size(); i++) {
        char c = inputStr[i];
        
        auto it = simplifiedDFA[state].trans.find(c);
        
        if(it == simplifiedDFA[state].trans.end()) {
            // No transition - STOP animation here
            trace->append(QString("<b style='color:red;'>❌ ERROR at position %1</b>").arg(i));
            trace->append(QString("   No transition for character '%1' from state q%2")
                .arg(QChar(c)).arg(state));
            trace->append("   This should have been caught by validation!");
            hasError = true;
            break;
        }
        
        int nextState = it->second;
        QString tokenContext;
        
        if(nextState == 1) tokenContext = "ID";
        else if(nextState == 2) tokenContext = "NUM";
        else if(nextState == 3) tokenContext = "OP";
        else if(nextState == 4) tokenContext = "WS";
        else tokenContext = "?";
        
        animationSteps.push_back({
            state, 
            nextState, 
            c, 
            (int)i,
            tokenContext
        });
        
        state = nextState;
    }
    
    if(hasError) {
        return;
    }
    
    animationStep = 0;
    dfaState = 0;
    dfaPos = 0;
    
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
        trace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        if(simplifiedDFA[dfaState].accept) {
            trace->append("<b style='color:green;'>✅ ACCEPTED!</b> Valid expression");
            
            QString stateLabel = "";
            if(dfaState == 1) stateLabel = "[ID]";
            else if(dfaState == 2) stateLabel = "[NUM]";
            else if(dfaState == 3) stateLabel = "[OP]";
            else if(dfaState == 4) stateLabel = "[WS]";
            
            trace->append(QString("   Final: q%1 %2 ✓").arg(dfaState).arg(stateLabel));
        } else {
            trace->append("<b style='color:red;'>❌ REJECTED!</b> Not in accept state");
            trace->append(QString("   Final: q%1 (non-accept)").arg(dfaState));
        }
        
        timer->stop();
        return;
    }

    auto &step = animationSteps[animationStep];
    
    dfaState = step.toState;
    dfaPos = step.inputPos;
    view->highlightState(dfaState);
    updateInputDisplay();

    QString charDisplay = (step.ch == ' ') ? "␣" : 
                         (step.ch == '\t') ? "⇥" : 
                         QString(QChar(step.ch));
    
    QString stateInfo = QString("q%1").arg(dfaState);
    
    if(simplifiedDFA[dfaState].accept) {
        QString category = "";
        if(dfaState == 1) category = "[ID]";
        else if(dfaState == 2) category = "[NUM]";
        else if(dfaState == 3) category = "[OP]";
        else if(dfaState == 4) category = "[WS]";
        stateInfo = QString("<span style='color:green;'>q%1%2 ✓</span>")
            .arg(dfaState).arg(category);
    }
    
    trace->append(QString("Step %1: q%2 --[%3]--> %4")
        .arg(animationStep + 1)
        .arg(step.fromState)
        .arg(charDisplay)
        .arg(stateInfo));
    
    animationStep++;
}

void MainWindow::onStepParse() {
    if(tokens.empty()) {
        outputParseBox->clear();
        outputParseBox->append("⚠ Please tokenize first (Tab 2).");
        return;
    }
    
    // Initialize on first step
    if(parser.getCurrentPosition() == 0 && parser.getStack().size() == 2) {
        if(outputParseBox->toPlainText().isEmpty() || 
           !outputParseBox->toPlainText().contains("Validation")) {
            outputParseBox->clear();
            parseTrace->clear();
            
            ValidationResult validation = ExpressionValidator::validate(tokens);
            if(!validation.valid) {
                outputParseBox->append("<b style='color:red;'>❌ VALIDATION FAILED</b>");
                outputParseBox->append(QString("Error: %1").arg(QString::fromStdString(validation.error)));
                return;
            }
            
            outputParseBox->append("✅ Validation passed.");
            outputParseBox->append("Click 'Step Parse' repeatedly to advance step-by-step.");
            outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            parser.reset();
            updateStack();
            updateParseInputDisplay();
            
            // Highlight ONLY start state initially
            for(auto &pair : allPDAStates) {
                QGraphicsEllipseItem *circle = pair.second;
                QString stateKey = QString::fromStdString(pair.first);
                QColor defaultColor;
                
                if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
                    defaultColor = QColor(200, 220, 255);
                } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
                    defaultColor = QColor(220, 200, 255);
                } else if(stateKey.contains("_F")) {
                    defaultColor = QColor(255, 220, 180);
                } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
                    defaultColor = QColor(200, 255, 200);
                } else if(stateKey.contains("accept")) {
                    defaultColor = QColor(144, 238, 144);
                } else {
                    defaultColor = QColor(220, 230, 255);
                }
                
                circle->setBrush(QBrush(defaultColor));
                circle->setPen(QPen(Qt::black, 2));
            }
            
            // Highlight ONLY start state
            if(allPDAStates.count("q_start")) {
                QGraphicsEllipseItem *startCircle = allPDAStates["q_start"];
                startCircle->setBrush(QBrush(QColor(255, 255, 100)));
                startCircle->setPen(QPen(Qt::red, 3));
            }
            
            return;
        }
    }
    
    // CRITICAL FIX: Check if parser is done AND stack is empty BEFORE stepping
    // This is to handle the final $ matching step
    if(parser.isDone() && parser.getStack().empty()) {
        // Already at accept state, don't process further
        if(!outputParseBox->toPlainText().contains("COMPLETE")) {
            outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            outputParseBox->append("<b style='color:green;'>✅ PARSE COMPLETE - ACCEPTED</b>");
            
            // Reset all states, highlight ONLY accept
            for(auto &pair : allPDAStates) {
                QGraphicsEllipseItem *circle = pair.second;
                QString stateKey = QString::fromStdString(pair.first);
                QColor defaultColor;
                
                if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
                    defaultColor = QColor(200, 220, 255);
                } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
                    defaultColor = QColor(220, 200, 255);
                } else if(stateKey.contains("_F")) {
                    defaultColor = QColor(255, 220, 180);
                } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
                    defaultColor = QColor(200, 255, 200);
                } else if(stateKey.contains("accept")) {
                    defaultColor = QColor(144, 238, 144);
                } else {
                    defaultColor = QColor(220, 230, 255);
                }
                
                circle->setBrush(QBrush(defaultColor));
                circle->setPen(QPen(Qt::black, 2));
            }
            
            // Highlight ONLY accept state
            if(allPDAStates.count("q_accept")) {
                QGraphicsEllipseItem *acceptCircle = allPDAStates["q_accept"];
                acceptCircle->setBrush(QBrush(QColor(50, 205, 50)));
                acceptCircle->setPen(QPen(Qt::darkGreen, 4));
            }
        }
        return;
    }
    
    // Perform one step
    auto stackBefore = parser.getStack();
    int posBefore = parser.getCurrentPosition();
    
    // SPECIAL CASE: If stack only has $, this is the final acceptance step
    if(stackBefore.size() == 1 && stackBefore[0] == "$") {
        parseTrace->append("Step: Match $ (end marker) - Parse complete!");
        
        // One more step to consume the final $
        bool success = parser.stepParse(tokens);
        
        if(success && parser.isDone() && parser.getStack().empty()) {
            outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            outputParseBox->append("<b style='color:green;'>✅ PARSE COMPLETE - ACCEPTED</b>");
            
            // Reset all states, highlight ONLY accept
            for(auto &pair : allPDAStates) {
                QGraphicsEllipseItem *circle = pair.second;
                QString stateKey = QString::fromStdString(pair.first);
                QColor defaultColor;
                
                if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
                    defaultColor = QColor(200, 220, 255);
                } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
                    defaultColor = QColor(220, 200, 255);
                } else if(stateKey.contains("_F")) {
                    defaultColor = QColor(255, 220, 180);
                } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
                    defaultColor = QColor(200, 255, 200);
                } else if(stateKey.contains("accept")) {
                    defaultColor = QColor(144, 238, 144);
                } else {
                    defaultColor = QColor(220, 230, 255);
                }
                
                circle->setBrush(QBrush(defaultColor));
                circle->setPen(QPen(Qt::black, 2));
            }
            
            // Highlight ONLY accept state
            if(allPDAStates.count("q_accept")) {
                QGraphicsEllipseItem *acceptCircle = allPDAStates["q_accept"];
                acceptCircle->setBrush(QBrush(QColor(50, 205, 50)));
                acceptCircle->setPen(QPen(Qt::darkGreen, 4));
            }
            
            updateStack();
            updateParseInputDisplay();
        }
        return;
    }
    
    bool success = parser.stepParse(tokens);
    
    auto stackAfter = parser.getStack();
    int posAfter = parser.getCurrentPosition();
    
    if(!success) {
        outputParseBox->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        outputParseBox->append("<b style='color:red;'>❌ PARSE ERROR - REJECTED</b>");
        parseTrace->append("Syntax error: unexpected token or invalid production");
        return;
    }
    
    // Determine state BEFORE stepping (to match animation)
    std::string stateId = "q_start";
    if(!stackBefore.empty()) {
        std::string top = stackBefore.back();
        
        if(top == "E") {
            stateId = "q_E";
        } else if(top == "E'") {
            stateId = "q_Ep";
        } else if(top == "T") {
            stateId = "q_T";
        } else if(top == "T'") {
            stateId = "q_Tp";
        } else if(top == "F") {
            stateId = "q_F";
        } else if(top == "ID") {
            stateId = "q_ID";
        } else if(top == "NUMBER") {
            stateId = "q_NUM";
        } else if(top == "+" || top == "-" || top == "*" || top == "/" || top == "(" || top == ")") {
            stateId = "q_OP";
        } else if(top == "$") {
            stateId = "q_accept";  // $ means we're going to accept
        }
    }
    
    // Reset ALL states to default
    for(auto &pair : allPDAStates) {
        QGraphicsEllipseItem *circle = pair.second;
        QString stateKey = QString::fromStdString(pair.first);
        QColor defaultColor;
        
        if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
            defaultColor = QColor(200, 220, 255);
        } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
            defaultColor = QColor(220, 200, 255);
        } else if(stateKey.contains("_F")) {
            defaultColor = QColor(255, 220, 180);
        } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
            defaultColor = QColor(200, 255, 200);
        } else if(stateKey.contains("accept")) {
            defaultColor = QColor(144, 238, 144);
        } else if(stateKey.contains("start")) {
            defaultColor = QColor(220, 230, 255);
        } else {
            defaultColor = QColor(220, 230, 255);
        }
        
        circle->setBrush(QBrush(defaultColor));
        circle->setPen(QPen(Qt::black, 2));
    }
    
    // Highlight ONLY the active state
    if(allPDAStates.count(stateId)) {
        QGraphicsEllipseItem *activeCircle = allPDAStates[stateId];
        activeCircle->setBrush(QBrush(QColor(255, 255, 100))); // Yellow
        activeCircle->setPen(QPen(Qt::red, 3));
    }
    
    // Log the step
    if(!stackBefore.empty()) {
        std::string top = stackBefore.back();
        
        if(top == "E" || top == "E'" || top == "T" || top == "T'" || top == "F") {
            parseTrace->append(QString("Step: Expand %1 → production applied")
                .arg(QString::fromStdString(top)));
            
            if(stackAfter.size() > stackBefore.size() - 1) {
                QString pushed;
                for(size_t i = stackBefore.size(); i < stackAfter.size(); i++) {
                    pushed += QString::fromStdString(stackAfter[i]) + " ";
                }
                parseTrace->append(QString("      Pushed: %1").arg(pushed.trimmed()));
            }
        } else {
            parseTrace->append(QString("Step: Match '%1' with token")
                .arg(QString::fromStdString(top)));
            if(posBefore < (int)tokens.size()) {
                parseTrace->append(QString("      Consumed: %1")
                    .arg(QString::fromStdString(tokens[posBefore].lexeme)));
            }
        }
    }
    
    updateStack();
    updateParseInputDisplay();
}

void MainWindow::onResetParse() {
    parser.reset();
    pdaAnimStep = 0;
    pdaTimer->stop();
    
    outputParseBox->clear();
    parseTrace->clear();
    parseInputDisplay->clear();
    
    updateStack();
    updatePDAState();
    
    outputParseBox->append("✅ Parser reset. Ready to parse.");
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
    
    // Display stack top to bottom (top at top of list)
    for(int i = stack.size() - 1; i >= 0; i--) {
        QString symbol = QString::fromStdString(stack[i]);
        QListWidgetItem *item = new QListWidgetItem(symbol);
        
        // Color code different symbol types
        if(symbol == "$") {
            item->setBackground(QColor(255, 200, 200)); // Light red for $
        } else if(symbol == "E" || symbol == "E'" || symbol == "T" || symbol == "T'" || symbol == "F") {
            item->setBackground(QColor(200, 220, 255)); // Light blue for non-terminals
        } else {
            item->setBackground(QColor(200, 255, 200)); // Light green for terminals
        }
        
        stackBox->addItem(item);
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
    // Check if animation is complete FIRST
    if(pdaAnimStep >= (int)pdaAnimationSteps.size()) {
        pdaTimer->stop();
        parseTrace->append("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        parseTrace->append("✅ PDA animation complete!");
        
        // CRITICAL FIX: Highlight accept state at the end
        // Reset ALL states first
        for(auto &pair : allPDAStates) {
            QGraphicsEllipseItem *circle = pair.second;
            QString stateKey = QString::fromStdString(pair.first);
            QColor defaultColor;
            
            if(stateKey.contains("_E") || stateKey.contains("_Ep")) {
                defaultColor = QColor(200, 220, 255);
            } else if(stateKey.contains("_T") || stateKey.contains("_Tp")) {
                defaultColor = QColor(220, 200, 255);
            } else if(stateKey.contains("_F")) {
                defaultColor = QColor(255, 220, 180);
            } else if(stateKey.contains("_ID") || stateKey.contains("_NUM") || stateKey.contains("_OP")) {
                defaultColor = QColor(200, 255, 200);
            } else if(stateKey.contains("accept")) {
                defaultColor = QColor(144, 238, 144);
            } else {
                defaultColor = QColor(220, 230, 255);
            }
            
            circle->setBrush(QBrush(defaultColor));
            circle->setPen(QPen(Qt::black, 2));
        }
        
        // Now highlight ONLY accept state
        if(allPDAStates.count("q_accept")) {
            QGraphicsEllipseItem *acceptCircle = allPDAStates["q_accept"];
            acceptCircle->setBrush(QBrush(QColor(50, 205, 50))); // Bright green
            acceptCircle->setPen(QPen(Qt::darkGreen, 4));
        }
        
        return;
    }
    
    auto &step = pdaAnimationSteps[pdaAnimStep];
    
    // CRITICAL FIX: Reset ALL states to default colors (including start!)
    for(auto &pair : allPDAStates) {
        QGraphicsEllipseItem *circle = pair.second;
        
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
        } else if(stateKey.contains("start")) {
            defaultColor = QColor(220, 230, 255); // Blue for start (NOT YELLOW!)
        } else {
            defaultColor = QColor(220, 230, 255); // Default blue
        }
        
        circle->setBrush(QBrush(defaultColor));
        circle->setPen(QPen(Qt::black, 2));
    }
    
    // Highlight ONLY the current state (one at a time!)
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
    
    // CRITICAL FIX: Step parser forward ONLY if not first or last step
    // Skip step 0 (initialization) and last step (already done)
    if(pdaAnimStep > 0 && pdaAnimStep < (int)pdaAnimationSteps.size() - 1) {
        if(!parser.isDone()) {
            parser.stepParse(tokens);
            updateStack();
            updateParseInputDisplay();
        }
    }
    
    pdaAnimStep++;
}

void MainWindow::buildSimplifiedDFA() {
    simplifiedDFA.clear();
    simplifiedDFA.resize(6);
    
    // ===============================================
    // q0: Start state
    // ===============================================
    simplifiedDFA[0].id = 0;
    simplifiedDFA[0].accept = false;
    
    // From q0: initial character determines token type
    for(char c = 'a'; c <= 'z'; c++) simplifiedDFA[0].trans[c] = 1;
    for(char c = 'A'; c <= 'Z'; c++) simplifiedDFA[0].trans[c] = 1;
    for(char c = '0'; c <= '9'; c++) simplifiedDFA[0].trans[c] = 2;
    simplifiedDFA[0].trans['+'] = 3;
    simplifiedDFA[0].trans['-'] = 3;
    simplifiedDFA[0].trans['*'] = 3;
    simplifiedDFA[0].trans['/'] = 3;
    simplifiedDFA[0].trans['('] = 3;
    simplifiedDFA[0].trans[')'] = 3;
    simplifiedDFA[0].trans[' '] = 4;
    simplifiedDFA[0].trans['\t'] = 4;
    
    // ===============================================
    // q1: ID state (ACCEPT)
    // ===============================================
    simplifiedDFA[1].id = 1;
    simplifiedDFA[1].accept = true;
    simplifiedDFA[1].tokens.push_back(TK_ID);
    
    // ID continues
    for(char c = 'a'; c <= 'z'; c++) simplifiedDFA[1].trans[c] = 1;
    for(char c = 'A'; c <= 'Z'; c++) simplifiedDFA[1].trans[c] = 1;
    for(char c = '0'; c <= '9'; c++) simplifiedDFA[1].trans[c] = 1;
    simplifiedDFA[1].trans['_'] = 1;
    
    // ID followed by other tokens
    simplifiedDFA[1].trans[' '] = 4;
    simplifiedDFA[1].trans['\t'] = 4;
    simplifiedDFA[1].trans['+'] = 3;
    simplifiedDFA[1].trans['-'] = 3;
    simplifiedDFA[1].trans['*'] = 3;
    simplifiedDFA[1].trans['/'] = 3;
    simplifiedDFA[1].trans['('] = 3;
    simplifiedDFA[1].trans[')'] = 3;
    
    // ===============================================
    // q2: NUMBER state (ACCEPT)
    // ===============================================
    simplifiedDFA[2].id = 2;
    simplifiedDFA[2].accept = true;
    simplifiedDFA[2].tokens.push_back(TK_NUMBER);
    
    // NUMBER continues
    for(char c = '0'; c <= '9'; c++) simplifiedDFA[2].trans[c] = 2;
    simplifiedDFA[2].trans['.'] = 2;
    
    // NUMBER followed by other tokens
    simplifiedDFA[2].trans[' '] = 4;
    simplifiedDFA[2].trans['\t'] = 4;
    simplifiedDFA[2].trans['+'] = 3;
    simplifiedDFA[2].trans['-'] = 3;
    simplifiedDFA[2].trans['*'] = 3;
    simplifiedDFA[2].trans['/'] = 3;
    simplifiedDFA[2].trans['('] = 3;
    simplifiedDFA[2].trans[')'] = 3;
    
    // ===============================================
    // q3: OPERATOR state (ACCEPT)
    // ===============================================
    simplifiedDFA[3].id = 3;
    simplifiedDFA[3].accept = true;
    simplifiedDFA[3].tokens.push_back(TK_PLUS);
    simplifiedDFA[3].tokens.push_back(TK_MINUS);
    simplifiedDFA[3].tokens.push_back(TK_STAR);
    simplifiedDFA[3].tokens.push_back(TK_SLASH);
    simplifiedDFA[3].tokens.push_back(TK_LPAREN);
    simplifiedDFA[3].tokens.push_back(TK_RPAREN);
    
    // OP followed by other tokens
    for(char c = 'a'; c <= 'z'; c++) simplifiedDFA[3].trans[c] = 1;
    for(char c = 'A'; c <= 'Z'; c++) simplifiedDFA[3].trans[c] = 1;
    for(char c = '0'; c <= '9'; c++) simplifiedDFA[3].trans[c] = 2;
    simplifiedDFA[3].trans[' '] = 4;
    simplifiedDFA[3].trans['\t'] = 4;
    simplifiedDFA[3].trans['+'] = 3;
    simplifiedDFA[3].trans['-'] = 3;
    simplifiedDFA[3].trans['*'] = 3;
    simplifiedDFA[3].trans['/'] = 3;
    simplifiedDFA[3].trans['('] = 3;
    simplifiedDFA[3].trans[')'] = 3;
    
    // ===============================================
    // q4: WHITESPACE state (ACCEPT)
    // ===============================================
    simplifiedDFA[4].id = 4;
    simplifiedDFA[4].accept = true;
    simplifiedDFA[4].tokens.push_back(TK_WS);
    
    // WS continues
    simplifiedDFA[4].trans[' '] = 4;
    simplifiedDFA[4].trans['\t'] = 4;
    
    // WS followed by other tokens
    for(char c = 'a'; c <= 'z'; c++) simplifiedDFA[4].trans[c] = 1;
    for(char c = 'A'; c <= 'Z'; c++) simplifiedDFA[4].trans[c] = 1;
    for(char c = '0'; c <= '9'; c++) simplifiedDFA[4].trans[c] = 2;
    simplifiedDFA[4].trans['+'] = 3;
    simplifiedDFA[4].trans['-'] = 3;
    simplifiedDFA[4].trans['*'] = 3;
    simplifiedDFA[4].trans['/'] = 3;
    simplifiedDFA[4].trans['('] = 3;
    simplifiedDFA[4].trans[')'] = 3;
    
    // Debug output
    qDebug() << "Built simplified DFA with" << simplifiedDFA.size() << "states";
    for(size_t i = 0; i < simplifiedDFA.size(); i++) {
        qDebug() << "  State q" << i << "- transitions:" << simplifiedDFA[i].trans.size()
                 << "accept:" << simplifiedDFA[i].accept;
    }
}