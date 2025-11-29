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
    
    QRectF rect = scene()->sceneRect();
    
    // Scale to fit with some margin
    fitInView(rect, Qt::KeepAspectRatio);
    
    // Ensure it's scaled down if needed
    qreal scaleX = viewport()->width() / rect.width();
    qreal scaleY = viewport()->height() / rect.height();
    qreal scale = qMin(scaleX, scaleY) * 0.95; // 95% to add margin
    
    resetTransform();
    QTransform transform;
    transform.scale(scale, scale);
    setTransform(transform);
    
    centerOn(rect.center());
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

    // FIXED: Check for 6 states (not 5!)
    if(dfa.size() <= 6) {
        buildSimplifiedLayout();
        return;
    }

    // Original BFS-based layout for complex DFAs
    buildBFSLayout();
}

void AutomataView::buildSimplifiedLayout() {
    // 5-state layout (no ERROR state)
    double startX = 150;
    double topY = 0;
    double spacingX = 200;
    double centerY = 250; // Center vertically
    
    // Create 5 nodes in a horizontal row
    createNode(0, 650, topY);                      // q0
    createNode(1, startX + spacingX, centerY);           // q1
    createNode(2, startX + spacingX * 2, centerY);       // q2  
    createNode(3, startX + spacingX * 3, centerY);       // q3
    createNode(4, startX + spacingX * 4, centerY);       // q4

    addStartArrow(0);

    // Collect all transitions
    std::map<std::pair<int, int>, std::vector<char>> edgeMap;
    for(size_t i = 0; i < dfa.size(); i++) {
        for(auto &kv : dfa[i].trans) {
            edgeMap[{(int)i, kv.second}].push_back(kv.first);
        }
    }

    std::set<std::pair<int, int>> drawnEdges;
    
    // PHASE 1: Self-loops
    for(int i = 0; i < 5; i++) {
        if(edgeMap.count({i, i})) {
            drawSelfLoop(i, edgeMap[{i, i}]);
            drawnEdges.insert({i, i});
        }
    }
    
    // PHASE 2: q0 outgoing
    for(int target = 1; target <= 4; target++) {
        if(edgeMap.count({0, target}) && !drawnEdges.count({0, target})) {
            drawDirectedEdge(0, target, edgeMap[{0, target}], false);
            drawnEdges.insert({0, target});
        }
    }
    
    // PHASE 3: Bidirectional between q1-q4 (CURVED!)
    std::vector<std::pair<int,int>> statePairs = {
        {1, 2}, {1, 3}, {1, 4},
        {2, 3}, {2, 4},
        {3, 4}
    };
    
    for(auto &pair : statePairs) {
        int a = pair.first;
        int b = pair.second;
        
        bool hasAB = edgeMap.count({a, b}) > 0;
        bool hasBA = edgeMap.count({b, a}) > 0;
        
        if(drawnEdges.count({a, b}) || drawnEdges.count({b, a})) {
            continue;
        }
        
        if(hasAB && hasBA) {
            // BIDIRECTIONAL with curves!
            drawBidirectionalEdgeClear(a, b, edgeMap[{a, b}], edgeMap[{b, a}]);
            drawnEdges.insert({a, b});
            drawnEdges.insert({b, a});
        } else if(hasAB) {
            bool shouldCurve = (std::abs(b - a) > 1);
            drawDirectedEdge(a, b, edgeMap[{a, b}], shouldCurve);
            drawnEdges.insert({a, b});
        } else if(hasBA) {
            bool shouldCurve = (std::abs(b - a) > 1);
            drawDirectedEdge(b, a, edgeMap[{b, a}], shouldCurve);
            drawnEdges.insert({b, a});
        }
    }
    
    // PHASE 4: Any remaining edges
    for(auto &kv : edgeMap) {
        int fromId = kv.first.first;
        int toId = kv.first.second;
        
        if(drawnEdges.count({fromId, toId})) continue;
        
        drawDirectedEdge(fromId, toId, kv.second, true);
        drawnEdges.insert({fromId, toId});
    }

    QRectF bounds = scene()->itemsBoundingRect();
    scene()->setSceneRect(bounds.adjusted(-50, -50, 50, 50));
    
    fitSceneInView();
}

void AutomataView::drawBidirectionalEdgeClear(int fromId, int toId,
                                               const std::vector<char> &forwardChars,
                                               const std::vector<char> &backwardChars) {
    if(!nodes.count(fromId) || !nodes.count(toId)) return;
    
    auto &from = nodes[fromId];
    auto &to = nodes[toId];

    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double distance = std::sqrt(dx*dx + dy*dy);
    double angle = std::atan2(dy, dx);
    double perpAngle = angle + M_PI/2;
    double radius = 30;
    
    // INCREASED separation for clearer visibility
    double offset = 35; // Was 30, now 35
    double curveAmount = std::max(50.0, distance * 0.25); // Stronger curve
    
    // ============================================
    // TOP ARROW (Blue) - curves upward
    // ============================================
    QPointF startF(from.x + radius * std::cos(angle) + offset * std::cos(perpAngle), 
                   from.y + radius * std::sin(angle) + offset * std::sin(perpAngle));
    QPointF endF(to.x - radius * std::cos(angle) + offset * std::cos(perpAngle), 
                 to.y - radius * std::sin(angle) + offset * std::sin(perpAngle));
    
    QPainterPath pathF;
    pathF.moveTo(startF);
    QPointF midF((startF.x() + endF.x())/2, (startF.y() + endF.y())/2);
    QPointF controlF(midF.x() + curveAmount * std::cos(perpAngle), 
                     midF.y() + curveAmount * std::sin(perpAngle));
    pathF.quadTo(controlF, endF);
    
    QPen forwardPen(QColor(70, 130, 220), 2.5);
    QGraphicsPathItem *itemF = scene()->addPath(pathF, forwardPen);
    edges.push_back(itemF);
    
    QPointF pEndF = pathF.pointAtPercent(0.95);
    QPointF pBeforeF = pathF.pointAtPercent(0.85);
    drawArrowHead(pBeforeF.x(), pBeforeF.y(), pEndF.x(), pEndF.y(), QColor(70, 130, 220));
    
    // ============================================
    // BOTTOM ARROW (Red) - curves downward
    // ============================================
    QPointF startB(to.x + radius * std::cos(angle + M_PI) - offset * std::cos(perpAngle), 
                   to.y + radius * std::sin(angle + M_PI) - offset * std::sin(perpAngle));
    QPointF endB(from.x - radius * std::cos(angle + M_PI) - offset * std::cos(perpAngle), 
                 from.y - radius * std::sin(angle + M_PI) - offset * std::sin(perpAngle));
    
    QPainterPath pathB;
    pathB.moveTo(startB);
    QPointF midB((startB.x() + endB.x())/2, (startB.y() + endB.y())/2);
    QPointF controlB(midB.x() - curveAmount * std::cos(perpAngle), 
                     midB.y() - curveAmount * std::sin(perpAngle));
    pathB.quadTo(controlB, endB);
    
    QPen backwardPen(QColor(220, 70, 70), 2.5);
    QGraphicsPathItem *itemB = scene()->addPath(pathB, backwardPen);
    edges.push_back(itemB);
    
    QPointF pEndB = pathB.pointAtPercent(0.95);
    QPointF pBeforeB = pathB.pointAtPercent(0.85);
    drawArrowHead(pBeforeB.x(), pBeforeB.y(), pEndB.x(), pEndB.y(), QColor(220, 70, 70));
    
    // ============================================
    // LABELS with white background
    // ============================================
    // QString forwardLabel = formatEdgeLabel(forwardChars);
    // QGraphicsTextItem *lblF = scene()->addText(forwardLabel);
    // QFont font = lblF->font();
    // font.setPointSize(8);
    // font.setBold(true);
    // lblF->setFont(font);
    // lblF->setDefaultTextColor(QColor(70, 130, 220));
    
    // QRectF lblBoundsF = lblF->boundingRect();
    // QGraphicsRectItem *bgF = scene()->addRect(
    //     lblBoundsF.adjusted(-2, -1, 2, 1),
    //     QPen(Qt::NoPen),
    //     QBrush(QColor(255, 255, 255, 230))
    // );
    
    // lblF->setPos(controlF.x() - lblBoundsF.width()/2, 
    //              controlF.y() - lblBoundsF.height()/2 - 15);
    // bgF->setPos(lblF->pos());
    // bgF->setZValue(-1);
    
    // edgeLabels.push_back(lblF);
    
    // QString backwardLabel = formatEdgeLabel(backwardChars);
    // QGraphicsTextItem *lblB = scene()->addText(backwardLabel);
    // lblB->setFont(font);
    // lblB->setDefaultTextColor(QColor(220, 70, 70));
    
    // QRectF lblBoundsB = lblB->boundingRect();
    // QGraphicsRectItem *bgB = scene()->addRect(
    //     lblBoundsB.adjusted(-2, -1, 2, 1),
    //     QPen(Qt::NoPen),
    //     QBrush(QColor(255, 255, 255, 230))
    // );
    
    // lblB->setPos(controlB.x() - lblBoundsB.width()/2, 
    //              controlB.y() - lblBoundsB.height()/2 + 15);
    // bgB->setPos(lblB->pos());
    // bgB->setZValue(-1);
    
    // edgeLabels.push_back(lblB);
}

void AutomataView::buildBFSLayout() {
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

    for(auto &lv : layers) {
        int y = verticalSpacing * (lv.first + 1);
        int nStates = lv.second.size();
        double horizontalSpacing = std::max(150.0, W / (nStates + 1.0));

        for(int i = 0; i < nStates; i++) {
            double x = horizontalSpacing * (i + 1);
            int stateId = lv.second[i];
            createNode(stateId, x, y);
        }
    }

    addStartArrow(0);

    std::map<std::pair<int, int>, std::vector<char>> edgeMap;
    for(size_t i = 0; i < dfa.size(); i++)
        for(auto &kv : dfa[i].trans)
            edgeMap[{(int)i, kv.second}].push_back(kv.first);

    for(auto &kv : edgeMap) {
        int fromId = kv.first.first;
        int toId = kv.first.second;
        auto &chars = kv.second;

        if(fromId == toId) {
            drawSelfLoop(fromId, chars);
        } else {
            drawDirectedEdge(fromId, toId, chars, false);
        }
    }

    scene()->setSceneRect(scene()->itemsBoundingRect());
    fitSceneInView();
}

void AutomataView::createNode(int stateId, double x, double y) {
    QGraphicsEllipseItem *e = scene()->addEllipse(x - 30, y - 30, 60, 60);
    e->setPen(QPen(Qt::black, 2));
    
    if(dfa[stateId].accept) {
        scene()->addEllipse(x - 25, y - 25, 50, 50, QPen(Qt::black, 2));
        e->setBrush(QBrush(QColor(144, 238, 144)));
    } else {
        e->setBrush(QBrush(Qt::white));
    }

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
    t->setPos(x - bounds.width()/2, y - bounds.height()/2);

    nodes[stateId] = {x, y, e, t};
}

void AutomataView::addStartArrow(int startState) {
    if(!nodes.count(startState)) return;
    
    auto &startNode = nodes[startState];
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

void AutomataView::drawSelfLoop(int stateId, const std::vector<char> &chars) {
    if(!nodes.count(stateId)) return;
    
    auto &node = nodes[stateId];
    double loopRadius = 20;
    
    QPainterPath path;
    path.moveTo(node.x - 10, node.y - 25);
    path.arcTo(node.x - loopRadius, node.y - 25 - loopRadius*2, 
               loopRadius*2, loopRadius*2, -30, 240);
    
    QPen edgePen(QColor(80, 80, 80), 2);
    QGraphicsPathItem *item = scene()->addPath(path, edgePen);
    edges.push_back(item);

    QPointF pEnd = path.pointAtPercent(0.95);
    QPointF pBefore = path.pointAtPercent(0.85);
    drawArrowHead(pBefore.x(), pBefore.y(), pEnd.x(), pEnd.y(), QColor(80, 80, 80));

    // QString labelText = formatEdgeLabel(chars);
    // QGraphicsTextItem *lbl = scene()->addText(labelText);
    // QFont font = lbl->font();
    // font.setPointSize(8);
    // lbl->setFont(font);
    // lbl->setDefaultTextColor(Qt::blue);
    
    // QRectF lblBounds = lbl->boundingRect();
    // lbl->setPos(node.x - lblBounds.width()/2, node.y - 80);
    // edgeLabels.push_back(lbl);
}

void AutomataView::drawDirectedEdge(int fromId, int toId, const std::vector<char> &chars, bool curved) {
    if(!nodes.count(fromId) || !nodes.count(toId)) return;
    
    auto &from = nodes[fromId];
    auto &to = nodes[toId];

    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double angle = std::atan2(dy, dx);
    double radius = 30;
    
    QPointF start(from.x + radius * std::cos(angle), from.y + radius * std::sin(angle));
    QPointF end(to.x - radius * std::cos(angle), to.y - radius * std::sin(angle));
    
    QPainterPath path;
    path.moveTo(start);
    
    if(curved) {
        QPointF mid((start.x() + end.x())/2, (start.y() + end.y())/2);
        double perpAngle = angle + M_PI/2;
        QPointF control(mid.x() + 30*std::cos(perpAngle), mid.y() + 30*std::sin(perpAngle));
        path.quadTo(control, end);
    } else {
        path.lineTo(end);
    }

    QPen edgePen(QColor(80, 80, 80), 2);
    QGraphicsPathItem *item = scene()->addPath(path, edgePen);
    edges.push_back(item);

    QPointF pEnd = path.pointAtPercent(0.95);
    QPointF pBefore = path.pointAtPercent(0.85);
    drawArrowHead(pBefore.x(), pBefore.y(), pEnd.x(), pEnd.y(), QColor(80, 80, 80));

    // QString labelText = formatEdgeLabel(chars);
    // QGraphicsTextItem *lbl = scene()->addText(labelText);
    // QFont font = lbl->font();
    // font.setPointSize(8);
    // lbl->setFont(font);
    // lbl->setDefaultTextColor(Qt::blue);
    
    // double midX = (start.x() + end.x())/2;
    // double midY = (start.y() + end.y())/2;
    // double perpAngle = angle + M_PI/2;
    // QPointF labelPos(midX + 20*std::cos(perpAngle), midY + 20*std::sin(perpAngle));
    
    // QRectF lblBounds = lbl->boundingRect();
    // lbl->setPos(labelPos.x() - lblBounds.width()/2, labelPos.y() - lblBounds.height()/2);
    // edgeLabels.push_back(lbl);
}

void AutomataView::drawBidirectionalEdge(int fromId, int toId, 
                                          const std::vector<char> &forwardChars,
                                          const std::vector<char> &backwardChars) {
    // This old function is kept for compatibility but not used
    // Use drawBidirectionalEdgeClear instead
    drawBidirectionalEdgeClear(fromId, toId, forwardChars, backwardChars);
}

void AutomataView::highlightState(int id) {
    for(auto &p : nodes) {
        int stateId = p.first;
        auto &n = p.second;
        
        if(dfa[stateId].accept) {
            n.e->setBrush(QBrush(QColor(144, 238, 144)));
            n.e->setPen(QPen(Qt::black, 2));
        } else {
            n.e->setBrush(QBrush(Qt::white));
            n.e->setPen(QPen(Qt::black, 2));
        }
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
    
    if(chars.size() == 1 && chars[0] == '*') {
        return "any";
    }
    
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
        return "alnum_";
    } else if(hasAllLower && hasAllUpper) {
        return "letter";
    } else if(hasAllDigits) {
        return "digit";
    }
    
    if(sorted.size() <= 6) {
        QString result;
        for(size_t i = 0; i < sorted.size(); i++) {
            char c = sorted[i];
            if(c == ' ') result += "␣";
            else if(c == '\t') result += "⇥";
            else if(c == '.') result += ".";
            else result += QChar(c);
            if(i < sorted.size() - 1) result += ",";
        }
        return result;
    }
    
    return QString("%1ch").arg(sorted.size());
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