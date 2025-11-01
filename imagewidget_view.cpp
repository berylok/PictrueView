#include "imagewidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QPainterPath>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>

void ImageWidget::paintEvent(QPaintEvent *event)
{
    if (currentViewMode == SingleView) {
        Q_UNUSED(event);
        QPainter painter(this);

        // 如果启用了透明背景，使用透明颜色填充
        if (testAttribute(Qt::WA_TranslucentBackground)) {
            painter.fillRect(rect(), QColor(0, 0, 0, 0));
        } else {
            // 非透明背景时填充黑色
            painter.fillRect(rect(), Qt::black);
        }

        // 安全检查：确保pixmap有效
        if (pixmap.isNull()) {
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, tr("没有图片或图片加载失败"));
            return;
        }

        // 安全检查：确保scaleFactor有效
        if (scaleFactor <= 0) {
            scaleFactor = 1.0;
        }

        QSize scaledSize = pixmap.size() * scaleFactor;

        // 安全检查：确保缩放后的尺寸有效
        if (scaledSize.width() <= 0 || scaledSize.height() <= 0) {
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, tr("图片尺寸无效"));
            return;
        }

        qDebug() << "绘制图片 - 原始尺寸:" << pixmap.size() << "缩放后:" << scaledSize;

        QPixmap scaledPixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 安全检查：确保缩放后的pixmap有效
        if (scaledPixmap.isNull()) {
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, tr("图片缩放失败"));
            return;
        }

        QPointF offset((width() - scaledPixmap.width()) / 2 + panOffset.x(),
                       (height() - scaledPixmap.height()) / 2 + panOffset.y());

        painter.drawPixmap(offset, scaledPixmap);

        // 添加变换状态提示
        if (isTransformed()) {
            painter.setPen(Qt::yellow);
            painter.setFont(QFont("Arial", 10));

            QString transformInfo;
            if (rotationAngle != 0) {
                transformInfo += QString("旋转: %1° ").arg(rotationAngle);
            }
            if (isHorizontallyFlipped) {
                transformInfo += "水平镜像 ";
            }
            if (isVerticallyFlipped) {
                transformInfo += "垂直镜像 ";
            }

            if (!transformInfo.isEmpty()) {
                painter.drawText(10, 30, transformInfo.trimmed());
            }
        }

        // 添加左右箭头提示（半透明，只在图片较大时显示）
        if (scaledSize.width() > 600 && scaledSize.height() > 600) {
            drawNavigationArrows(painter, offset, scaledSize);
        }

        qDebug() << "图片绘制完成";
    }
}

void ImageWidget::drawNavigationArrows(QPainter &painter, const QPointF &offset,
                                       const QSize &scaledSize)
{
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(0.5); // 半透明

    // 左箭头
    QPainterPath leftArrow;
    leftArrow.moveTo(offset.x() + 20, offset.y() + scaledSize.height() / 2);
    leftArrow.lineTo(offset.x() + 40, offset.y() + scaledSize.height() / 2 - 15);
    leftArrow.lineTo(offset.x() + 40, offset.y() + scaledSize.height() / 2 + 15);
    leftArrow.closeSubpath();
    painter.fillPath(leftArrow, Qt::white);

    // 右箭头
    QPainterPath rightArrow;
    rightArrow.moveTo(offset.x() + scaledSize.width() - 20, offset.y() + scaledSize.height() / 2);
    rightArrow.lineTo(offset.x() + scaledSize.width() - 40, offset.y() + scaledSize.height() / 2 - 15);
    rightArrow.lineTo(offset.x() + scaledSize.width() - 40, offset.y() + scaledSize.height() / 2 + 15);
    rightArrow.closeSubpath();
    painter.fillPath(rightArrow, Qt::white);

    painter.setOpacity(1.0); // 恢复不透明
}

bool ImageWidget::shouldShowNavigationArrows(const QSize &scaledSize)
{
    return scaledSize.width() > 600 && scaledSize.height() > 600;
}

void ImageWidget::fitToWindow()
{
    if (pixmap.isNull()) return;

    QSize windowSize = this->size();
    if (windowSize.isEmpty() || windowSize.width() <= 0 || windowSize.height() <= 0) {
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect desktopRect = screen->availableGeometry();
        windowSize = desktopRect.size();
    }

    QSize imageSize = pixmap.size();
    double widthRatio = static_cast<double>(windowSize.width()) / imageSize.width();
    double heightRatio = static_cast<double>(windowSize.height()) / imageSize.height();

    scaleFactor = qMin(widthRatio, heightRatio);
    panOffset = QPointF(0, 0);
    currentViewStateType = FitToWindow; // 设置为合适大小模式

    update();
}

void ImageWidget::actualSize()
{
    if (pixmap.isNull()) return;

    scaleFactor = 1.0;
    panOffset = QPointF(0, 0);
    currentViewStateType = ActualSize; // 设置为实际大小模式

    update();
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    if (currentViewMode == SingleView) {
        // 标记为手动调整
        currentViewStateType = ManualAdjustment;

        double zoomFactor = 1.15;
        double oldScaleFactor = scaleFactor;

        if (event->angleDelta().y() > 0) {
            scaleFactor *= zoomFactor;
        } else {
            scaleFactor /= zoomFactor;
        }

        scaleFactor = qBound(0.03, scaleFactor, 8.0);

        QPointF mousePos = event->position();
        QPointF imagePos =
            (mousePos - QPointF(width() / 2, height() / 2) - panOffset) /
            oldScaleFactor;
        panOffset = mousePos - QPointF(width() / 2, height() / 2) -
                    imagePos * scaleFactor;

        update();
    }
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (currentViewMode == ThumbnailView) {
        thumbnailWidget->update();
    }
}
