#ifndef AUTOMATAVIEW_H
#define AUTOMATAVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QResizeEvent>
#include <map>
#include <vector>
#include "core/dfa.h"

class AutomataView : public QGraphicsView {
    Q_OBJECT

public:
    explicit AutomataView(QWidget *parent = nullptr);
    
    void buildFromDFA(const std::vector<DFAState> &dfa_);
    void highlightState(int id);
    void highlightTransition(int fromId, int toId, char c);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Node { 
        double x, y; 
        QGraphicsEllipseItem *e; 
        QGraphicsTextItem *t; 
    };
    
    std::map<int, Node> nodes;
    std::vector<QGraphicsPathItem*> edges;
    std::vector<QGraphicsTextItem*> edgeLabels;
    int activeState = -1;
    std::vector<DFAState> dfa;
    
    // Layout functions
    void buildSimplifiedLayout();
    void buildBFSLayout();
    void fitSceneInView();
    
    // Node creation
    void createNode(int stateId, double x, double y);
    void addStartArrow(int startState);
    
    // Edge drawing functions
    void drawSelfLoop(int stateId, const std::vector<char> &chars);
    void drawDirectedEdge(int fromId, int toId, const std::vector<char> &chars, bool curved);
    void drawBidirectionalEdge(int fromId, int toId, 
                              const std::vector<char> &forwardChars,
                              const std::vector<char> &backwardChars);
    
    // NEW: Clearer bidirectional with better separation
    void drawBidirectionalEdgeClear(int fromId, int toId,
                                    const std::vector<char> &forwardChars,
                                    const std::vector<char> &backwardChars);
    
    // Utilities
    QString formatEdgeLabel(const std::vector<char> &chars);
    void drawArrowHead(double x1, double y1, double x2, double y2, QColor color = QColor(80, 80, 80));
};

#endif // AUTOMATAVIEW_H