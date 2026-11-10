#ifndef AUTOMATONGRAPHEDITOR_H
#define AUTOMATONGRAPHEDITOR_H

#include "domain/automaton.h"

#include <QGraphicsView>
#include <QJsonObject>
#include <QPointF>
#include <QVector>

class GraphEdgeItem;
class GraphVertexItem;

struct GraphVertexData {
    int id = -1;
    QString label;
    QPointF position;
};

struct GraphEdgeData {
    int fromId = -1;
    int toId = -1;
    QString label;
    QPointF controlOffset;
};

class AutomatonGraphEditor : public QGraphicsView {
    Q_OBJECT
public:
    explicit AutomatonGraphEditor(QWidget* parent = nullptr);
    static QRectF workspaceRect();

    void setTemplate(const Automaton& automaton);
    void clearGraph();
    void createVertex();
    void deleteLastVertex();

    QVector<GraphVertexData> vertices() const;
    QVector<GraphEdgeData> edges() const;
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& object, QString* error = nullptr);

signals:
    void graphChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    friend class GraphEdgeItem;
    friend class GraphVertexItem;

    GraphVertexItem* vertexAt(const QPoint& viewportPosition) const;
    GraphVertexItem* selectedVertex() const;
    GraphEdgeItem* createEdge(GraphVertexItem* from, GraphVertexItem* to,
                              const QString& label = QString(),
                              const QPointF& controlOffset = QPointF());
    void removeEdge(GraphEdgeItem* edge);
    void removeVertex(GraphVertexItem* vertex);
    void editVertex(GraphVertexItem* vertex);
    void editEdge(GraphEdgeItem* edge);
    void updatePalette();
    void clearItems();
    QString nextVertexLabel() const;
    QPointF nextVertexPosition() const;

    QGraphicsScene* m_scene;
    QVector<GraphVertexItem*> m_vertices;
    QVector<GraphEdgeItem*> m_edges;
    AutomatonKind m_kind = AutomatonKind::Mealy;
    int m_nextVertexId = 1;
};

#endif
