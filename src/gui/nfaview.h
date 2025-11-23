#ifndef NFAVIEW_H
#define NFAVIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <map>
#include <vector>
#include "core/nfa.h"

class NFAView : public QGraphicsView {
    Q_OBJECT

public:
    explicit NFAView(QWidget *parent = nullptr);
    
    void buildFromNFA(const NFAFragment &fragment, const FullNFA &nfa, const QString &label);
    void clear();

private:
    struct Node {
        double x, y;
        QGraphicsEllipseItem *e;
        QGraphicsTextItem *t;
    };
    
    std::map<int, Node> nodes;
    std::vector<QGraphicsPathItem*> edges;
    
    void drawArrowHead(double x1, double y1, double x2, double y2, QColor color = QColor(80, 80, 80));
    void drawState(int stateId, double x, double y, bool isStart, bool isAccept, const QString &label);
    void drawTransition(int from, int to, const QString &label, const FullNFA &nfa);
};

#endif // NFAVIEW_H