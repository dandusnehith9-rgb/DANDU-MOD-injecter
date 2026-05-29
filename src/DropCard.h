#pragma once

#include <QFrame>

class QPropertyAnimation;

class DropCard : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(qreal glow READ glow WRITE setGlow)

public:
    explicit DropCard(QWidget *parent = nullptr);

    qreal glow() const;
    void setGlow(qreal value);

signals:
    void fileDropped(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void animateGlow(qreal target);

    qreal m_glow = 0.0;
    QPropertyAnimation *m_glowAnim = nullptr;
};
