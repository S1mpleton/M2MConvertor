#include "ui/automatongrapheditor.h"

#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QInputDialog>
#include <QHash>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLineF>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPalette>
#include <QStyleOptionGraphicsItem>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr qreal kVertexRadius = 34.0;

QPointF normalized(const QPointF& value) {
    const qreal length = std::hypot(value.x(), value.y());
    return length > 0.001 ? value / length : QPointF(1.0, 0.0);
}

QString statePart(const QString& label) {
    return label.section(QLatin1Char('/'), 0, 0).trimmed();
}
}

class GraphVertexItem final : public QGraphicsEllipseItem {
public:
    GraphVertexItem(AutomatonGraphEditor* owner, int id, const QString& label)
        : QGraphicsEllipseItem(-kVertexRadius, -kVertexRadius,
                               kVertexRadius * 2, kVertexRadius * 2),
          m_owner(owner), m_id(id), m_label(label),
          m_text(new QGraphicsSimpleTextItem(this)) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setCacheMode(DeviceCoordinateCache);
        setZValue(2.0);
        m_text->setAcceptedMouseButtons(Qt::NoButton);
        updateText();
    }

    int id() const { return m_id; }
    QString label() const { return m_label; }
    void setLabel(const QString& value) {
        m_label = value.trimmed();
        updateText();
    }
    void setInitial(bool value) { m_initial = value; update(); }
    void setColors(const QColor& outline, const QColor& fill,
                   const QColor& text) {
        m_outline = outline;
        m_fill = fill;
        m_text->setBrush(text);
        update();
    }
    void attach(GraphEdgeItem* edge) { if (!m_edges.contains(edge)) m_edges.append(edge); }
    void detach(GraphEdgeItem* edge) { m_edges.removeAll(edge); }
    const QVector<GraphEdgeItem*>& attachedEdges() const { return m_edges; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        m_owner->editVertex(this);
        event->accept();
    }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget*) override {
        QColor outline = m_outline;
        QPen pen(outline, option->state & QStyle::State_Selected ? 3.0 : 2.0);
        if (option->state & QStyle::State_Selected) pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(m_fill);
        painter->drawEllipse(rect());
        if (m_initial)
            painter->drawEllipse(rect().adjusted(5, 5, -5, -5));
    }

private:
    void updateText() {
        m_text->setText(m_label.isEmpty() ? QStringLiteral("?") : m_label);
        const QRectF bounds = m_text->boundingRect();
        m_text->setPos(-bounds.width() / 2.0, -bounds.height() / 2.0);
    }

    AutomatonGraphEditor* m_owner;
    int m_id;
    QString m_label;
    QGraphicsSimpleTextItem* m_text;
    QVector<GraphEdgeItem*> m_edges;
    bool m_initial = false;
    QColor m_outline;
    QColor m_fill;
};

class GraphEdgeItem final : public QGraphicsPathItem {
public:
    GraphEdgeItem(AutomatonGraphEditor* owner, GraphVertexItem* from,
                  GraphVertexItem* to, const QString& label,
                  const QPointF& controlOffset)
        : m_owner(owner), m_from(from), m_to(to), m_label(label),
          m_controlOffset(controlOffset),
          m_labelBackground(new QGraphicsRectItem(this)),
          m_text(new QGraphicsSimpleTextItem(this)),
          m_arrow(new QGraphicsPolygonItem(this)) {
        setFlags(ItemIsSelectable);
        setZValue(1.0);
        setCursor(Qt::SizeAllCursor);
        m_text->setAcceptedMouseButtons(Qt::NoButton);
        m_labelBackground->setAcceptedMouseButtons(Qt::NoButton);
        m_arrow->setAcceptedMouseButtons(Qt::NoButton);
        m_labelBackground->setZValue(1.0);
        m_arrow->setZValue(1.0);
        m_text->setZValue(2.0);
        m_from->attach(this);
        m_to->attach(this);
        updateGeometry();
    }

    GraphVertexItem* from() const { return m_from; }
    GraphVertexItem* to() const { return m_to; }
    QString label() const { return m_label; }
    QPointF controlOffset() const { return m_controlOffset; }
    void setLabel(const QString& value) {
        m_label = value.trimmed();
        updateGeometry();
    }
    void setControlOffset(const QPointF& value) {
        m_controlOffset = value;
        updateGeometry();
    }
    void setColors(const QColor& edge, const QColor& text,
                   const QColor& labelBackground) {
        m_edgeColor = edge;
        m_text->setBrush(text);
        m_labelBackground->setBrush(labelBackground);
        m_labelBackground->setPen(QPen(edge, 0.8));
        m_arrow->setBrush(edge);
        m_arrow->setPen(QPen(edge));
        update();
    }
    void updateGeometry() {
        prepareGeometryChange();
        QPainterPath curve;
        QPointF arrowDirection;
        if (m_from == m_to) {
            QPointF offset = m_controlOffset;
            if (std::hypot(offset.x(), offset.y()) < 30.0)
                offset = QPointF(0.0, -85.0);
            const QPointF direction = normalized(offset);
            const QPointF side(-direction.y(), direction.x());
            const QPointF center = m_from->scenePos();
            const QPointF start = center + direction * kVertexRadius - side * 15.0;
            const QPointF end = center + direction * kVertexRadius + side * 15.0;
            const QPointF farPoint = center + offset;
            curve.moveTo(start);
            curve.cubicTo(farPoint - side * 50.0, farPoint + side * 50.0, end);
            arrowDirection = normalized(end - (farPoint + side * 50.0));
        } else {
            const QPointF first = m_from->scenePos();
            const QPointF second = m_to->scenePos();
            const QPointF control = (first + second) / 2.0 + m_controlOffset;
            const QPointF start = first + normalized(control - first) * kVertexRadius;
            const QPointF end = second - normalized(second - control) * kVertexRadius;
            curve.moveTo(start);
            curve.quadTo(control, end);
            arrowDirection = normalized(end - control);
        }
        setPath(curve);

        const QPointF tip = curve.pointAtPercent(1.0);
        const QPointF side(-arrowDirection.y(), arrowDirection.x());
        QPolygonF arrow;
        arrow << tip
              << tip - arrowDirection * 12.0 + side * 6.0
              << tip - arrowDirection * 12.0 - side * 6.0;
        m_arrow->setPolygon(arrow);

        m_text->setText(m_label.isEmpty() ? QStringLiteral("?") : m_label);
        const QPointF labelPosition = curve.pointAtPercent(0.5);
        const QRectF textBounds = m_text->boundingRect();
        const QPointF textPosition(labelPosition.x() - textBounds.width() / 2.0,
                                   labelPosition.y() - textBounds.height() / 2.0);
        m_text->setPos(textPosition);
        m_labelBackground->setRect(textBounds.adjusted(-4, -2, 4, 2));
        m_labelBackground->setPos(textPosition);
    }

    QPainterPath shape() const override {
        QPainterPathStroker stroker;
        stroker.setWidth(14.0);
        return stroker.createStroke(path());
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            if (!event->modifiers().testFlag(Qt::ControlModifier) && scene())
                scene()->clearSelection();
            m_dragging = true;
            setSelected(true);
            event->accept();
            return;
        }
        QGraphicsPathItem::mousePressEvent(event);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
        if (!m_dragging) {
            QGraphicsPathItem::mouseMoveEvent(event);
            return;
        }
        const QPointF base = m_from == m_to
            ? m_from->scenePos()
            : (m_from->scenePos() + m_to->scenePos()) / 2.0;
        setControlOffset(event->scenePos() - base);
        m_owner->updateSceneBounds();
        m_owner->graphChanged();
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        m_dragging = false;
        QGraphicsPathItem::mouseReleaseEvent(event);
    }
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        m_owner->editEdge(this);
        event->accept();
    }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget*) override {
        QPen pen(m_edgeColor,
                 option->state & QStyle::State_Selected ? 3.0 : 1.8);
        if (option->state & QStyle::State_Selected) pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path());
    }

private:
    AutomatonGraphEditor* m_owner;
    GraphVertexItem* m_from;
    GraphVertexItem* m_to;
    QString m_label;
    QPointF m_controlOffset;
    QGraphicsRectItem* m_labelBackground;
    QGraphicsSimpleTextItem* m_text;
    QGraphicsPolygonItem* m_arrow;
    QColor m_edgeColor;
    bool m_dragging = false;
};

QVariant GraphVertexItem::itemChange(GraphicsItemChange change,
                                     const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        for (GraphEdgeItem* edge : std::as_const(m_edges)) edge->updateGeometry();
        m_owner->updateSceneBounds();
        m_owner->graphChanged();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

AutomatonGraphEditor::AutomatonGraphEditor(QWidget* parent)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this)) {
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setViewportUpdateMode(BoundingRectViewportUpdate);
    setDragMode(RubberBandDrag);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(380);
    m_scene->setSceneRect(-500, -350, 1000, 700);
    updatePalette();
}

void AutomatonGraphEditor::setTemplate(const Automaton& automaton) {
    m_suggestedLabels.clear();
    m_kind = automaton.kind;
    m_initialStateName = automaton.states.value(automaton.initialState);
    for (int state = 0; state < automaton.states.size(); ++state) {
        QString label = automaton.states[state];
        if (automaton.kind == AutomatonKind::Moore) {
            QStringList outputs;
            for (int output : automaton.mooreOutputs.value(state))
                outputs.append(automaton.outputs.value(output));
            label += QStringLiteral(" / ") +
                     (outputs.isEmpty() ? QStringLiteral("-") : outputs.join(','));
        }
        m_suggestedLabels.append(label);
    }
    updateInitialMarkers();
}

void AutomatonGraphEditor::clearGraph() {
    clearItems();
    m_suggestedLabels.clear();
    m_initialStateName.clear();
}

void AutomatonGraphEditor::clearItems() {
    m_vertices.clear();
    m_edges.clear();
    m_scene->clear();
    m_nextVertexId = 1;
    emit graphChanged();
}

QString AutomatonGraphEditor::nextVertexLabel() const {
    QStringList used;
    for (const GraphVertexItem* vertex : m_vertices) used.append(vertex->label());
    for (const QString& suggestion : m_suggestedLabels)
        if (!used.contains(suggestion)) return suggestion;
    int number = 1;
    for (;;) {
        const QString candidate = m_kind == AutomatonKind::Moore
            ? QStringLiteral("q%1 / -").arg(number)
            : QStringLiteral("q%1").arg(number);
        if (!used.contains(candidate)) return candidate;
        ++number;
    }
}

QPointF AutomatonGraphEditor::nextVertexPosition() const {
    const int index = m_vertices.size();
    const qreal angle = index * 2.399963229728653;
    const qreal radius = 55.0 + 32.0 * std::sqrt(qreal(index));
    return QPointF(std::cos(angle) * radius, std::sin(angle) * radius);
}

void AutomatonGraphEditor::createVertex() {
    auto* vertex = new GraphVertexItem(this, m_nextVertexId++, nextVertexLabel());
    vertex->setPos(nextVertexPosition());
    m_scene->addItem(vertex);
    m_vertices.append(vertex);
    m_scene->clearSelection();
    vertex->setSelected(true);
    updatePalette();
    updateInitialMarkers();
    updateSceneBounds();
    centerOn(vertex);
    setFocus();
    emit graphChanged();
}

GraphVertexItem* AutomatonGraphEditor::vertexAt(const QPoint& viewportPosition) const {
    QGraphicsItem* item = itemAt(viewportPosition);
    while (item && !dynamic_cast<GraphVertexItem*>(item)) item = item->parentItem();
    return dynamic_cast<GraphVertexItem*>(item);
}

GraphVertexItem* AutomatonGraphEditor::selectedVertex() const {
    for (QGraphicsItem* item : m_scene->selectedItems())
        if (auto* vertex = dynamic_cast<GraphVertexItem*>(item)) return vertex;
    return nullptr;
}

GraphEdgeItem* AutomatonGraphEditor::createEdge(GraphVertexItem* from,
                                                GraphVertexItem* to,
                                                const QString& label,
                                                const QPointF& requestedOffset) {
    if (!from || !to) return nullptr;
    QPointF offset = requestedOffset;
    if (offset.isNull()) {
        int parallel = 0;
        for (const GraphEdgeItem* edge : m_edges) {
            if ((edge->from() == from && edge->to() == to) ||
                (edge->from() == to && edge->to() == from))
                ++parallel;
        }
        if (from == to) {
            offset = QPointF(0, -85 - parallel * 28);
        } else if (parallel > 0) {
            const QPointF direction = normalized(to->scenePos() - from->scenePos());
            const QPointF normal(-direction.y(), direction.x());
            const qreal distance = 32.0 * ((parallel + 1) / 2.0) *
                                   (parallel % 2 == 0 ? -1.0 : 1.0);
            offset = normal * distance;
        }
    }
    auto* edge = new GraphEdgeItem(this, from, to, label, offset);
    m_scene->addItem(edge);
    m_edges.append(edge);
    updatePalette();
    emit graphChanged();
    return edge;
}

void AutomatonGraphEditor::removeEdge(GraphEdgeItem* edge) {
    if (!edge) return;
    edge->from()->detach(edge);
    edge->to()->detach(edge);
    m_edges.removeAll(edge);
    m_scene->removeItem(edge);
    delete edge;
    updateSceneBounds();
    emit graphChanged();
}

void AutomatonGraphEditor::removeVertex(GraphVertexItem* vertex) {
    if (!vertex) return;
    const QVector<GraphEdgeItem*> attached = vertex->attachedEdges();
    for (GraphEdgeItem* edge : attached)
        if (m_edges.contains(edge)) removeEdge(edge);
    m_vertices.removeAll(vertex);
    m_scene->removeItem(vertex);
    delete vertex;
    updateSceneBounds();
    emit graphChanged();
}

void AutomatonGraphEditor::deleteSelectedVertex() {
    removeVertex(selectedVertex());
    setFocus();
}

void AutomatonGraphEditor::editVertex(GraphVertexItem* vertex) {
    bool accepted = false;
    const QString hint = m_kind == AutomatonKind::Mealy
        ? tr("Имя состояния, например q1")
        : tr("Состояние / выходы, например q1 / y1,y2");
    const QString value = QInputDialog::getText(
        this, tr("Подпись вершины"), hint, QLineEdit::Normal,
        vertex->label(), &accepted).trimmed();
    if (accepted && !value.isEmpty()) {
        vertex->setLabel(value);
        updateInitialMarkers();
        updateSceneBounds();
        emit graphChanged();
    }
}

void AutomatonGraphEditor::editEdge(GraphEdgeItem* edge) {
    bool accepted = false;
    const QString hint = m_kind == AutomatonKind::Mealy
        ? tr("Вход / выходы; например x1 / y1 или x1 / -; x2 / y2")
        : tr("Входы через «;», например x1; x2");
    const QString value = QInputDialog::getText(
        this, tr("Подпись дуги"), hint, QLineEdit::Normal,
        edge->label(), &accepted).trimmed();
    if (accepted) {
        edge->setLabel(value);
        updateSceneBounds();
        emit graphChanged();
    }
}

void AutomatonGraphEditor::updateInitialMarkers() {
    for (GraphVertexItem* vertex : m_vertices)
        vertex->setInitial(statePart(vertex->label()) == m_initialStateName);
}

void AutomatonGraphEditor::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton &&
        event->modifiers().testFlag(Qt::ControlModifier)) {
        GraphVertexItem* source = selectedVertex();
        GraphVertexItem* target = vertexAt(event->position().toPoint());
        if (source && target) {
            GraphEdgeItem* edge = createEdge(source, target);
            m_scene->clearSelection();
            edge->setSelected(true);
            event->accept();
            setFocus();
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
    setFocus();
}

void AutomatonGraphEditor::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        if (GraphVertexItem* vertex = selectedVertex()) {
            removeVertex(vertex);
        } else {
            const QList<QGraphicsItem*> selected = m_scene->selectedItems();
            for (QGraphicsItem* item : selected) {
                if (auto* edge = dynamic_cast<GraphEdgeItem*>(item)) {
                    removeEdge(edge);
                    break;
                }
            }
        }
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void AutomatonGraphEditor::changeEvent(QEvent* event) {
    QGraphicsView::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) updatePalette();
}

void AutomatonGraphEditor::updatePalette() {
    setBackgroundBrush(palette().brush(QPalette::Base));
    const QColor outline = palette().color(QPalette::Highlight);
    const QColor fill = palette().color(QPalette::AlternateBase);
    const QColor background = palette().color(QPalette::Base);
    const QColor text = palette().color(QPalette::Text);
    for (GraphVertexItem* vertex : m_vertices)
        vertex->setColors(outline, fill, text);
    for (GraphEdgeItem* edge : m_edges)
        edge->setColors(text, text, background);
}

void AutomatonGraphEditor::updateSceneBounds() {
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-140, -140, 140, 140);
    const QRectF minimum(-500, -350, 1000, 700);
    if (bounds.isEmpty()) bounds = minimum;
    m_scene->setSceneRect(bounds.united(minimum));
}

QVector<GraphVertexData> AutomatonGraphEditor::vertices() const {
    QVector<GraphVertexData> result;
    for (const GraphVertexItem* vertex : m_vertices)
        result.append({vertex->id(), vertex->label(), vertex->scenePos()});
    return result;
}

QVector<GraphEdgeData> AutomatonGraphEditor::edges() const {
    QVector<GraphEdgeData> result;
    for (const GraphEdgeItem* edge : m_edges) {
        result.append({edge->from()->id(), edge->to()->id(), edge->label(),
                       edge->controlOffset()});
    }
    return result;
}

QJsonObject AutomatonGraphEditor::toJson() const {
    QJsonArray vertexArray;
    for (const GraphVertexData& vertex : vertices()) {
        vertexArray.append(QJsonObject{
            {QStringLiteral("id"), vertex.id},
            {QStringLiteral("label"), vertex.label},
            {QStringLiteral("x"), vertex.position.x()},
            {QStringLiteral("y"), vertex.position.y()}});
    }
    QJsonArray edgeArray;
    for (const GraphEdgeData& edge : edges()) {
        edgeArray.append(QJsonObject{
            {QStringLiteral("from"), edge.fromId},
            {QStringLiteral("to"), edge.toId},
            {QStringLiteral("label"), edge.label},
            {QStringLiteral("controlX"), edge.controlOffset.x()},
            {QStringLiteral("controlY"), edge.controlOffset.y()}});
    }
    return QJsonObject{{QStringLiteral("vertices"), vertexArray},
                       {QStringLiteral("edges"), edgeArray}};
}

bool AutomatonGraphEditor::fromJson(const QJsonObject& object, QString* error) {
    clearItems();
    const auto fail = [this, error](const QString& message) {
        if (error) *error = message;
        clearItems();
        return false;
    };
    const QJsonValue vertexValue = object.value(QStringLiteral("vertices"));
    const QJsonValue edgeValue = object.value(QStringLiteral("edges"));
    if (!vertexValue.isArray() || !edgeValue.isArray())
        return fail(tr("Повреждены данные графического редактора."));
    QHash<int, GraphVertexItem*> byId;
    for (const QJsonValue& value : vertexValue.toArray()) {
        if (!value.isObject())
            return fail(tr("Повреждена вершина сохранённого графа."));
        const QJsonObject item = value.toObject();
        const int id = item.value(QStringLiteral("id")).toInt(-1);
        const QString label = item.value(QStringLiteral("label")).toString();
        if (id < 0 || label.trimmed().isEmpty() || byId.contains(id))
            return fail(tr("Повреждена вершина сохранённого графа."));
        auto* vertex = new GraphVertexItem(this, id, label);
        vertex->setPos(item.value(QStringLiteral("x")).toDouble(),
                       item.value(QStringLiteral("y")).toDouble());
        m_scene->addItem(vertex);
        m_vertices.append(vertex);
        byId.insert(id, vertex);
        m_nextVertexId = qMax(m_nextVertexId, id + 1);
    }
    for (const QJsonValue& value : edgeValue.toArray()) {
        if (!value.isObject())
            return fail(tr("Повреждена дуга сохранённого графа."));
        const QJsonObject item = value.toObject();
        GraphVertexItem* from = byId.value(item.value(QStringLiteral("from")).toInt(-1));
        GraphVertexItem* to = byId.value(item.value(QStringLiteral("to")).toInt(-1));
        if (!from || !to)
            return fail(tr("Дуга сохранённого графа ссылается на неизвестную вершину."));
        createEdge(from, to, item.value(QStringLiteral("label")).toString(),
                   QPointF(item.value(QStringLiteral("controlX")).toDouble(),
                           item.value(QStringLiteral("controlY")).toDouble()));
    }
    updatePalette();
    updateInitialMarkers();
    updateSceneBounds();
    emit graphChanged();
    return true;
}
