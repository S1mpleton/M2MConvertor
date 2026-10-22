#include "ui/automatoncanvas.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QResizeEvent>
#include <QtMath>

#include <cmath>

AutomatonCanvas::AutomatonCanvas(QWidget* parent)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setMinimumHeight(280);
    setBackgroundBrush(palette().brush(QPalette::Base));
}

void AutomatonCanvas::clearAutomaton() {
    m_scene->clear();
}

void AutomatonCanvas::setAutomaton(const Automaton& automaton) {
    m_scene->clear();
    const int count = automaton.states.size();
    if (count == 0) return;
    const qreal radius = qMax<qreal>(120.0, 30.0 * count);
    QVector<QPointF> centers;
    for (int i = 0; i < count; ++i) {
        const qreal angle = -M_PI_2 + (2.0 * M_PI * i / count);
        centers.append(QPointF(radius * qCos(angle), radius * qSin(angle)));
    }
    const qreal nodeRadius = 30.0;
    const QColor edgeColor = palette().color(QPalette::Text);
    const QColor nodeColor = palette().color(QPalette::Highlight);
    const QColor nodeFill = palette().color(QPalette::AlternateBase);
    const QColor textColor = palette().color(QPalette::Text);
    for (const Transition& edge : automaton.transitions) {
        const QPointF from = centers[edge.from];
        const QPointF to = centers[edge.to];
        QStringList outputs;
        for (int output : edge.outputs) outputs.append(automaton.outputs[output]);
        const QString label = automaton.inputs[edge.input] +
            (outputs.isEmpty() ? QString() : QStringLiteral(" / ") + outputs.join(','));
        if (edge.from == edge.to) {
            const QRectF loop(from.x() - 22, from.y() - 72, 44, 48);
            m_scene->addEllipse(loop, QPen(edgeColor, 1.6));
            auto* text = m_scene->addText(label);
            text->setPos(from.x() - text->boundingRect().width() / 2, from.y() - 98);
            continue;
        }
        QLineF direction(from, to);
        direction.setLength(qMax<qreal>(0, direction.length() - nodeRadius));
        QLineF line(direction.p1(), direction.p2());
        line.setP1(direction.pointAt(nodeRadius / QLineF(from, to).length()));
        m_scene->addLine(line, QPen(edgeColor, 1.6));
        const qreal angle = std::atan2(-line.dy(), line.dx());
        const QPointF tip = line.p2();
        QPolygonF arrow;
        arrow << tip
              << tip + QPointF(std::sin(angle + M_PI / 3) * 10,
                               std::cos(angle + M_PI / 3) * 10)
              << tip + QPointF(std::sin(angle + M_PI - M_PI / 3) * 10,
                               std::cos(angle + M_PI - M_PI / 3) * 10);
        m_scene->addPolygon(arrow, QPen(Qt::NoPen), QBrush(edgeColor));
        auto* text = m_scene->addText(label);
        const QPointF middle = line.pointAt(0.5);
        text->setPos(middle.x() - text->boundingRect().width() / 2,
                     middle.y() - text->boundingRect().height() - 3);
    }
    for (int state = 0; state < count; ++state) {
        QString title = automaton.states[state];
        if (automaton.kind == AutomatonKind::Moore) {
            QStringList outputs;
            for (int output : automaton.mooreOutputs[state])
                outputs.append(automaton.outputs[output]);
            title += QStringLiteral(" / ") +
                     (outputs.isEmpty() ? QStringLiteral("—") : outputs.join(','));
        }
        const QRectF rect(centers[state].x() - nodeRadius,
                          centers[state].y() - nodeRadius,
                          nodeRadius * 2, nodeRadius * 2);
        m_scene->addEllipse(rect, QPen(nodeColor, 2), QBrush(nodeFill));
        if (state == automaton.initialState)
            m_scene->addEllipse(rect.adjusted(4, 4, -4, -4), QPen(nodeColor, 1));
        auto* text = m_scene->addText(title);
        text->setDefaultTextColor(textColor);
        text->setPos(centers[state].x() - text->boundingRect().width() / 2,
                     centers[state].y() - text->boundingRect().height() / 2);
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30));
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void AutomatonCanvas::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (!m_scene->items().isEmpty()) fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}
