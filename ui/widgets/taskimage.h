#pragma once
#include <QImage>
#include <QEvent>
#include <QPainter>
#include <QWidget>

// Keep the original image; fit it to the current viewport on every paint/resize.
class TaskImage : public QWidget {
    Q_OBJECT
  public:
    explicit TaskImage(QWidget* parent = nullptr) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(0, 80);
    }
    void setImageData(const QByteArray& data) {
        m_image = QImage::fromData(data);
        update();
    }
    QSize sizeHint() const override {
        return QSize(480, 180);
    }
    QRect imageRect() const {
        if (m_image.isNull())
            return {};
        const auto size = m_image.size().scaled(contentsRect().size(), Qt::KeepAspectRatio);
        return QRect(contentsRect().topLeft() +
                         QPoint((contentsRect().width() - size.width()) / 2,
                                (contentsRect().height() - size.height()) / 2),
                     size);
    }

  protected:
    void changeEvent(QEvent* event) override {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::LanguageChange) update();
    }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        if (m_image.isNull()) {
            painter.setPen(palette().color(QPalette::WindowText));
            painter.drawText(rect(), Qt::AlignCenter, tr("Изображение задания отсутствует"));
        } else {
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawImage(imageRect(), m_image);
        }
    }

  private:
    QImage m_image;
};
