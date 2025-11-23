#include "automataview.h"
#include "core/tokens.h"
#include <queue>
#include <set>
#include <algorithm>
#include <cmath>

AutomataView::AutomataView(QWidget *parent) : QGraphicsView(parent) {
    setScene(new QGraphicsScene(this));
    setRenderHint(QPainter::Antialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void AutomataView::fitSceneInView() {
    if(!scene() || scene()->items().isEmpty()) return;
    QRectF rect = scene()->itemsBoundingRect().adjusted(-100, -100, 100, 100);
    fitInView(rect, Qt::KeepAspectRatio);
}

void AutomataView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    fitSceneInView();
}

void AutomataView::buildFromDFA(const std::vector<DFAState> &dfa_) {
    dfa = dfa_;
    scene()->clear();
    nodes.clear();
    edges.clear();
    edgeLabels.clear();
    if(dfa.empty()) return;

    // BFS for layers
    std::map<int, int> layer;
    std::queue<int> q;
    std::set<int> visited;
    q.push(0);
    layer[0] = 0;
    visited.insert(0);

    while(!q.empty()) {
        int s = q.front();
        q.pop();
        int l = layer[s];
        for(auto &kv : dfa[s].trans) {
            int t = kv.second;
            if(!visited.count(t)) {
                visited.insert(t);
                layer[t] = l + 1;
                q.push(t);
            }
        }
    }

    std::map<int, std::vector<int>> layers;
    int maxLayer = 0;
    for(auto &p : layer) {
        layers[p.second].push_back(p.first);
        maxLayer = std::max(maxLayer, p.second);
    }

    int W = 1600, H = 800;
    int verticalSpacing = std::max(150, H / (maxLayer + 2));

    // Create nodes
    for(auto &lv : layers) {
        int y = verticalSpacing * (lv.first + 1);
        int nStates = lv.second.size();
        double horizontalSpacing = std::max(150.0, W / (nStates + 1.0));

        for(int i = 0; i < nStates; i++) {
            double x = horizontalSpacing * (i + 1);
            int stateId = lv.second[i];

            // Create state circle
            QGraphicsEllipseItem *e = scene()->addEllipse(x - 30, double(y) - 30, 60, 60);
            e->setPen(QPen(Qt::black, 2));
            e->setBrush(QBrush(Qt::white));

            // Double circle for accept states
            if(dfa[stateId].accept) {
                scene()->addEllipse(x - 25, double(y) - 25, 50, 50, QPen(Qt::black, 2));
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

            QGraphicsTextItem *t = scene()->addText(labelText);
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
        auto &startNode = nodes[0];
        double arrowStartX = startNode.x - 60;
        double arrowStartY = startNode.y;
        QLineF line(arrowStartX, arrowStartY, startNode.x - 32, startNode.y);
        scene()->addLine(line, QPen(Qt::darkBlue, 3));
        drawArrowHead(arrowStartX + 20, arrowStartY, startNode.x - 32, startNode.y, Qt::darkBlue);
        
        QGraphicsTextItem *startLabel = scene()->addText("START");
        QFont font = startLabel->font();
        font.setPointSize(8);
        font.setBold(true);
        startLabel->setFont(font);
        startLabel->setDefaultTextColor(Qt::darkBlue);
        startLabel->setPos(arrowStartX - 10, arrowStartY - 25);
    }

    // Group transitions
    std::map<std::pair<int, int>, std::vector<char>> edgeMap;
    for(size_t i = 0; i < dfa.size(); i++)
        for(auto &kv : dfa[i].trans)
            edgeMap[{(int)i, kv.second}].push_back(kv.first);

    // Draw edges
    for(auto &kv : edgeMap) {
        int fromId = kv.first.first;
        int toId = kv.first.second;
        auto &chars = kv.second;

        auto &from = nodes[fromId];
        auto &to = nodes[toId];

        QPainterPath path;
        QPointF labelPos;
        
        if(fromId == toId) {
            double loopRadius = 35;
            path.moveTo(from.x - 10, from.y - 25);
            path.arcTo(from.x - loopRadius, from.y - 25 - loopRadius*2, loopRadius*2, loopRadius*2, -30, 240);
            labelPos = QPointF(from.x, from.y - 80);
        } else {
            double dx = to.x - from.x;
            double dy = to.y - from.y;
            double angle = std::atan2(dy, dx);
            double radius = 30;
            QPointF start(from.x + radius * std::cos(angle), from.y + radius * std::sin(angle));
            QPointF end(to.x - radius * std::cos(angle), to.y - radius * std::sin(angle));
            
            path.moveTo(start);
            path.lineTo(end);
            
            double midX = (start.x() + end.x())/2;
            double midY = (start.y() + end.y())/2;
            double perpAngle = angle + M_PI/2;
            labelPos = QPointF(midX + 20*std::cos(perpAngle), midY + 20*std::sin(perpAngle));
        }

        QPen edgePen(QColor(80, 80, 80), 2);
        QGraphicsPathItem *item = scene()->addPath(path, edgePen);
        edges.push_back(item);

        QPointF pEnd = path.pointAtPercent(0.95);
        QPointF pBefore = path.pointAtPercent(0.85);
        drawArrowHead(pBefore.x(), pBefore.y(), pEnd.x(), pEnd.y(), QColor(80, 80, 80));

        QString labelText = formatEdgeLabel(chars);
        QGraphicsTextItem *lbl = scene()->addText(labelText);
        QFont font = lbl->font();
        font.setPointSize(8);
        lbl->setFont(font);
        lbl->setDefaultTextColor(Qt::blue);
        
        QRectF lblBounds = lbl->boundingRect();
        lbl->setPos(labelPos.x() - lblBounds.width()/2, labelPos.y() - lblBounds.height()/2);
        
        edgeLabels.push_back(lbl);
    }

    scene()->setSceneRect(scene()->itemsBoundingRect());
    fitSceneInView();
}

void AutomataView::highlightState(int id) {
    for(auto &p : nodes) {
        int stateId = p.first;
        auto &n = p.second;
        bool isAccept = dfa[stateId].accept;
        n.e->setBrush(isAccept ? QBrush(QColor(144, 238, 144)) : QBrush(Qt::white));
        n.e->setPen(QPen(Qt::black, 2));
    }
    
    if(id >= 0 && nodes.count(id)) {
        nodes[id].e->setBrush(QBrush(QColor(255, 255, 100)));
        nodes[id].e->setPen(QPen(Qt::red, 3));
    }
    activeState = id;
}

void AutomataView::highlightTransition(int fromId, int toId, char c) {
    for(auto *edge : edges) {
        edge->setPen(QPen(QColor(80, 80, 80), 2));
    }
    
    if(nodes.count(fromId) && nodes.count(toId)) {
        highlightState(toId);
    }
}

QString AutomataView::formatEdgeLabel(const std::vector<char> &chars) {
    if(chars.empty()) return "";
    
    std::vector<char> sorted = chars;
    std::sort(sorted.begin(), sorted.end());
    std::set<char> charSet(sorted.begin(), sorted.end());
    
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

void AutomataView::drawArrowHead(double x1, double y1, double x2, double y2, QColor color) {
    QLineF line(QPointF(x1, y1), QPointF(x2, y2));
    double angle = std::atan2(-line.dy(), line.dx());
    const double arrowSize = 12;
    QPointF p1 = line.p2() - QPointF(arrowSize*std::cos(angle+M_PI/6), -arrowSize*std::sin(angle+M_PI/6));
    QPointF p2 = line.p2() - QPointF(arrowSize*std::cos(angle-M_PI/6), -arrowSize*std::sin(angle-M_PI/6));
    QPolygonF arrowHead;
    arrowHead << line.p2() << p1 << p2;
    scene()->addPolygon(arrowHead, QPen(color), QBrush(color));
}