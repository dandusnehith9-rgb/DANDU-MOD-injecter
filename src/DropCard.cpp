#include "DropCard.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEasingCurve>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>
#include <QUrl>

DropCard::DropCard(QWidget *parent)
    : QFrame(parent),
      m_glowAnim(new QPropertyAnimation(this, "glow", this))
{
    setAcceptDrops(true);
    setMinimumHeight(220);

    m_glowAnim->setDuration(220);
    m_glowAnim->setEasingCurve(QEasingCurve::OutCubic);
}

qreal DropCard::glow() const
{
    return m_glow;
}

void DropCard::setGlow(qreal value)
{
    m_glow = qBound(0.0, value, 1.0);
    update();
}

void DropCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF area = rect().adjusted(8, 8, -8, -8);
    const qreal radius = 22.0;

    QLinearGradient gradient(area.topLeft(), area.bottomRight());
    gradient.setColorAt(0.0, QColor(20, 28, 43));
    gradient.setColorAt(1.0, QColor(29, 36, 52));

    const QColor borderBase(82, 112, 170);
    const QColor borderGlow(56, 189, 248);
    const QColor border = QColor::fromRgbF(
        borderBase.redF() + (borderGlow.redF() - borderBase.redF()) * m_glow,
        borderBase.greenF() + (borderGlow.greenF() - borderBase.greenF()) * m_glow,
        borderBase.blueF() + (borderGlow.blueF() - borderBase.blueF()) * m_glow,
        0.85);

    p.setBrush(gradient);
    p.setPen(QPen(border, 2.0));
    p.drawRoundedRect(area, radius, radius);

    const QColor glowColor(56, 189, 248, static_cast<int>(130 * m_glow));
    p.setPen(Qt::NoPen);
    p.setBrush(glowColor);
    p.drawRoundedRect(area.adjusted(5, 5, -5, -5), radius - 5, radius - 5);

    p.setPen(QColor(230, 236, 244));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() + 4.0);
    p.setFont(titleFont);
    p.drawText(area.adjusted(0, 38, 0, 0), Qt::AlignHCenter, "Drop Mod .jar Here");

    p.setPen(QColor(189, 204, 224));
    QFont sub = font();
    sub.setPointSizeF(sub.pointSizeF() + 1.0);
    p.setFont(sub);
    p.drawText(area.adjusted(0, 95, 0, 0),
               Qt::AlignHCenter,
               "DANDU MOD injecter auto-detects NeoForge, Fabric, and Quilt");
}

void DropCard::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() != 1 || !urls.first().isLocalFile())
    {
        event->ignore();
        return;
    }

    const QString path = urls.first().toLocalFile();
    if (!path.endsWith(".jar", Qt::CaseInsensitive))
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    animateGlow(1.0);
}

void DropCard::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    animateGlow(0.0);
}

void DropCard::dropEvent(QDropEvent *event)
{
    animateGlow(0.0);

    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty() || !urls.first().isLocalFile())
    {
        event->ignore();
        return;
    }

    const QString path = urls.first().toLocalFile();
    if (!QFileInfo::exists(path) || !path.endsWith(".jar", Qt::CaseInsensitive))
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    emit fileDropped(path);
}

void DropCard::animateGlow(qreal target)
{
    m_glowAnim->stop();
    m_glowAnim->setStartValue(m_glow);
    m_glowAnim->setEndValue(target);
    m_glowAnim->start();
}
