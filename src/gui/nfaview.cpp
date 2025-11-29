#include "nfaview.h"
#include "core/tokens.h"
#include <QPainterPath>
#include <QFont>
#include <cmath>
#include <set>
#include <queue>

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

void NFAView::buildCombinedNFA(const FullNFA &nfa) {
    clear();
    
    if(nfa.states.empty()) return;
    
    // Group states by "depth" from start using BFS
    std::map<int, int> depth;
    std::queue<int> q;
    std::set<int> visited;
    
    q.push(nfa.start);
    depth[nfa.start] = 0;
    visited.insert(nfa.start);
    
    while(!q.empty()) {
        int curr = q.front();
        q.pop();
        
        for(const auto &trans : nfa.states[curr].trans) {
            int next = trans.to;
            if(!visited.count(next)) {
                visited.insert(next);
                depth[next] = depth[curr] + 1;
                q.push(next);
            }
        }
    }
    
    // Group states by depth
    std::map<int, std::vector<int>> layers;
    int maxDepth = 0;
    for(const auto &p : depth) {
        layers[p.second].push_back(p.first);
        maxDepth = std::max(maxDepth, p.second);
    }
    
    // Layout
    double width = 1200;
    double height = 400;
    double layerSpacing = width / (maxDepth + 2);
    
    std::map<int, std::pair<double, double>> positions;
    
    for(const auto &layer : layers) {
        int layerNum = layer.first;
        const auto &stateList = layer.second;
        double x = layerSpacing * (layerNum + 1);
        double stateSpacing = height / (stateList.size() + 1);
        
        for(size_t i = 0; i < stateList.size(); i++) {
            int stateId = stateList[i];
            double y = stateSpacing * (i + 1);
            positions[stateId] = {x, y};
            
            // Determine if this is an accept state
            bool isAccept = nfa.acceptToken.count(stateId) > 0;
            bool isStart = (stateId == nfa.start);
            
            QString label = QString("q%1").arg(stateId);
            if(isAccept) {
                int tokenId = nfa.acceptToken.at(stateId);
                label += QString("\n[%1]").arg(QString::fromStdString(tokenNames[tokenId]));
            }
            
            drawState(stateId, x, y, isStart, isAccept, label);
        }
    }
    
    // Draw all transitions
    std::set<std::pair<int, int>> drawnEdges;
    
    for(const auto &state : nfa.states) {
        int fromId = state.id;
        if(!positions.count(fromId)) continue;
        
        for(const auto &trans : state.trans) {
            int toId = trans.to;
            if(!positions.count(toId)) continue;
            
            // Skip if already drawn
            if(drawnEdges.count({fromId, toId})) continue;
            drawnEdges.insert({fromId, toId});
            
            // Get transition label
            QString transLabel;
            if(trans.kind == L_EPS) {
                transLabel = "ε";
            } else if(trans.kind == L_CHAR) {
                transLabel = QString(QChar(trans.ch));
            } else if(trans.kind == L_DIGIT) {
                transLabel = "[0-9]";
            } else if(trans.kind == L_LETTER) {
                transLabel = "[a-z, A-Z]";
            } else if(trans.kind == L_ALNUM_UNDERSCORE) {
                transLabel = "[alnum_]";
            }
            
            drawTransition(fromId, toId, transLabel, nfa);
        }
    }
    
    
    // Add info box
    QGraphicsTextItem *info = scene()->addText(
        QString("States: %1 | Start: q%2 | Accept States: %3")
            .arg(nfa.states.size())
            .arg(nfa.start)
            .arg(nfa.acceptToken.size())
    );
    QFont infoFont = info->font();
    infoFont.setPointSize(8);
    info->setFont(infoFont);
    info->setDefaultTextColor(QColor(100, 100, 100));
    info->setPos(20, height + 20);
    
    scene()->setSceneRect(scene()->itemsBoundingRect().adjusted(-30, -30, 30, 30));
}