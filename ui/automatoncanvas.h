#ifndef AUTOMATONCANVAS_H
#define AUTOMATONCANVAS_H

#include "domain/automaton.h"

#include <QGraphicsView>

class AutomatonCanvas : public QGraphicsView {
    Q_OBJECT
public:
    explicit AutomatonCanvas(QWidget* parent = nullptr);
    void setAutomaton(const Automaton& automaton);
    void clearAutomaton();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene;
};

#endif
