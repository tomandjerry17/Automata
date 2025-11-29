// mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QTableWidget>
#include <vector>
#include <string>
#include <map>

#include "core/nfa.h"
#include "core/dfa.h"
#include "core/tokens.h"
#include "parser/parser.h"
#include "automataview.h"
#include "nfaview.h"

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onLex();
    void onParse();
    void onAnimateDFA();
    void onStopDFA();
    void onResetDFA();
    void dfaStepTimer();
    void onStepParse();
    void onResetParse();
    void onClearOutput();
    void onBuildNFA();
    void pdaStepTimer();

private:
    // UI Components
    QTabWidget *tabWidget;
    QLineEdit *input;
    QListWidget *tokensBox;
    QListWidget *stackBox;
    QTextEdit *trace;
    QTextEdit *parseTrace;
    QTextEdit *grammarDisplay;
    AutomataView *view;
    QGraphicsView *pdaView;
    QTimer *timer;
    QLabel *inputDisplay;
    QLabel *parseInputDisplay;
    QLabel *dfaInfo;
    QTableWidget *parsingTableWidget;
    
    // Backend data
    FullNFA nfa;
    std::vector<DFAState> dfa;
    std::vector<Token> tokens;
    std::string cur;
    std::vector<NFAView*> nfaViews;

    void buildSimplifiedDFA();
    std::vector<DFAState> simplifiedDFA;
    
    // DFA Animation
    int dfaState = 0;
    int dfaPos = 0;
    
    struct AnimationStep {
        int fromState;
        int toState;
        char ch;
        int inputPos;
        QString tokenName;
    };
    std::vector<AnimationStep> animationSteps;
    int animationStep = 0;
    int currentTokenIndex = 0;
    
    // Parser state
    Parser parser;
    
    // PDA visualization
    int pdaState = 0;
    int pdaPhase = 0;   
    QGraphicsEllipseItem *pdaQ0Circle = nullptr;
    QGraphicsEllipseItem *pdaQ1Circle = nullptr;

    // PDA Animation
    struct PDAAnimationStep {
        QString stateName;
        QString action;
        QString stackOp;
        std::string stateId;
    };
    std::vector<PDAAnimationStep> pdaAnimationSteps;
    std::map<std::string, QGraphicsEllipseItem*> allPDAStates;
    int pdaAnimStep = 0;
    QTimer *pdaTimer;
    
    // Helper functions
    void resetDFA();
    void updatePDAState();
    void resetParse();
    void updateStack();
    void updateInputDisplay();
    void updateParseInputDisplay();
    void drawPDA();
};

#endif // MAINWINDOW_H