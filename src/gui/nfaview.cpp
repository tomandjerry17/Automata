#include "nfaview.h"
#include <QPainterPath>
#include <QFont>
#include <cmath>
#include <set>

NFAView::NFAView(QWidget *parent) : QGraphicsView(parent) {
    setScene(new QGraphicsScene(this));
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(150);
    setMaximumHeight(200);
}

void NFAView::clear() {
    scene()->clear();
    nodes.clear();
    edges.clear();
}

void NFAView::buildFromNFA(const NFAFragment &fragment, const FullNFA &nfa, const QString &label) {
    clear();
    
    // Simple layout: start on left, accept on right
    double startX = 100;
    double endX = 400;
    double y = 100;
    
    // Draw start state
    drawState(fragment.start, startX, y, true, false, QString("q%1\nSTART").arg(fragment.start));
    
    // Draw accept state
    drawState(fragment.accept, endX, y, false, true, QString("q%1\nACCEPT").arg(fragment.accept));
    
    // Draw transition with label
    drawTransition(fragment.start, fragment.accept, label, nfa);
    
    // Add title
    QGraphicsTextItem *title = scene()->addText(QString("NFA for: %1").arg(label));
    QFont titleFont = title->font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setPos(150, 20);
    
    scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-20, -20, 20, 20));
}

void NFAView::drawState(int stateId, double x, double y, bool isStart, bool isAccept, const QString &labelText) {
    QGraphicsEllipseItem *e = scene()->addEllipse(x - 25, y - 25, 50, 50);
    e->setPen(QPen(Qt::black, 2));
    
    if(isAccept) {
        e->setBrush(QBrush(QColor(144, 238, 144))); // Green
        scene()->addEllipse(x - 20, y - 20, 40, 40, QPen(Qt::black, 2));
    } else {
        e->setBrush(QBrush(Qt::white));
    }
    
    QGraphicsTextItem *t = scene()->addText(labelText);
    QFont font = t->font();
    font.setPointSize(8);
    t->setFont(font);
    QRectF bounds = t->boundingRect();
    t->setPos(x - bounds.width()/2, y - bounds.height()/2);
    
    nodes[stateId] = {x, y, e, t};
    
    // Draw start arrow if needed
    if(isStart) {
        double arrowStartX = x - 60;
        QLineF line(arrowStartX, y, x - 27, y);
        scene()->addLine(line, QPen(Qt::darkBlue, 2));
        drawArrowHead(arrowStartX + 20, y, x - 27, y, Qt::darkBlue);
    }
}

void NFAView::drawTransition(int from, int to, const QString &label, const FullNFA &nfa) {
    auto &fromNode = nodes[from];
    auto &toNode = nodes[to];
    
    // Straight line
    double angle = std::atan2(toNode.y - fromNode.y, toNode.x - fromNode.x);
    double radius = 25;
    QPointF start(fromNode.x + radius * std::cos(angle), fromNode.y + radius * std::sin(angle));
    QPointF end(toNode.x - radius * std::cos(angle), toNode.y - radius * std::sin(angle));
    
    QPainterPath path;
    path.moveTo(start);
    path.lineTo(end);
    
    QGraphicsPathItem *item = scene()->addPath(path, QPen(QColor(80, 80, 80), 2));
    edges.push_back(item);
    
    // Draw arrow head
    drawArrowHead(start.x() + (end.x() - start.x()) * 0.8, 
                  start.y() + (end.y() - start.y()) * 0.8,
                  end.x(), end.y(), QColor(80, 80, 80));
    
    // Label
    QGraphicsTextItem *lbl = scene()->addText(label);
    QFont font = lbl->font();
    font.setPointSize(9);
    lbl->setFont(font);
    lbl->setDefaultTextColor(Qt::blue);
    QRectF lblBounds = lbl->boundingRect();
    lbl->setPos((start.x() + end.x())/2 - lblBounds.width()/2, 
                (start.y() + end.y())/2 - lblBounds.height() - 10);
}

void NFAView::drawArrowHead(double x1, double y1, double x2, double y2, QColor color) {
    QLineF line(QPointF(x1, y1), QPointF(x2, y2));
    double angle = std::atan2(-line.dy(), line.dx());
    const double arrowSize = 10;
    QPointF p1 = line.p2() - QPointF(arrowSize*std::cos(angle+M_PI/6), -arrowSize*std::sin(angle+M_PI/6));
    QPointF p2 = line.p2() - QPointF(arrowSize*std::cos(angle-M_PI/6), -arrowSize*std::sin(angle-M_PI/6));
    QPolygonF arrowHead;
    arrowHead << line.p2() << p1 << p2;
    scene()->addPolygon(arrowHead, QPen(color), QBrush(color));
}