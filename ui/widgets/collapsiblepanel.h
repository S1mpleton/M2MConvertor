#pragma once
#include <QGroupBox>
#include <QToolButton>
#include <QVBoxLayout>

class CollapsiblePanel : public QGroupBox {
    Q_OBJECT
  public:
    CollapsiblePanel(const QString& title, QWidget* content, const QString& name,
                     QWidget* parent = nullptr)
        : QGroupBox(title, parent), m_content(content) {
        setObjectName(name);
        auto* layout = new QVBoxLayout(this);
        m_toggle = new QToolButton(this);
        m_toggle->setObjectName(name + "Toggle");
        m_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_toggle->setCheckable(true);
        m_toggle->setChecked(true);
        layout->addWidget(m_toggle, 0, Qt::AlignRight);
        layout->addWidget(content, 1);
        connect(m_toggle, &QToolButton::toggled, this, [this](bool expanded) {
            m_content->setVisible(expanded);
            m_toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
            m_toggle->setText(expanded ? tr("Скрыть подсказку") : tr("Показать подсказку"));
            updateGeometry();
        });
        m_toggle->setArrowType(Qt::DownArrow);
        m_toggle->setText(tr("Скрыть подсказку"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    }
    bool expanded() const {
        return m_toggle->isChecked();
    }
    void setExpanded(bool value) {
        m_toggle->setChecked(value);
    }

  private:
    QWidget* m_content;
    QToolButton* m_toggle;
};
