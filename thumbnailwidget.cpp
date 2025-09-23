#include "thumbnailwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFileInfo>
#include <QApplication>
#include <QFutureWatcher>
#include <QtConcurrent>

// 初始化静态成员变量
QMap<QString, QPixmap> ThumbnailWidget::thumbnailCache;
QMutex ThumbnailWidget::cacheMutex;

ThumbnailWidget::ThumbnailWidget(QWidget *parent)
    : QWidget(parent),
    thumbnailSize(150, 150),
    thumbnailSpacing(10),
    selectedIndex(-1),
    loadedCount(0),
    totalCount(0),
    futureWatcher(nullptr),
    isLoading(false)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

ThumbnailWidget::~ThumbnailWidget()
{
    stopLoading();
}

void ThumbnailWidget::setImageList(const QStringList &list, const QDir &dir)
{
    // 停止之前的加载
    stopLoading();

    imageList = list;
    currentDir = dir;
    selectedIndex = -1;
    loadedCount = 0;
    totalCount = list.size();

    update();

    emit loadingProgress(0, totalCount);

    // 检查哪些图片已经在缓存中
    QStringList pathsToLoad;
    {
        QMutexLocker locker(&cacheMutex);
        for (const QString &fileName : imageList) {
            QString imagePath = currentDir.absoluteFilePath(fileName);
            if (!thumbnailCache.contains(imagePath)) {
                pathsToLoad.append(imagePath);
            } else {
                loadedCount++;
            }
        }
    }

    // 如果有需要加载的图片
    if (!pathsToLoad.isEmpty()) {
        isLoading = true;

        // 使用 QtConcurrent 异步加载缩略图
        QFuture<QPixmap> future = QtConcurrent::mapped(pathsToLoad, [this](const QString &path) {
            return loadThumbnail(path);
        });

        // 创建监视器来跟踪进度
        futureWatcher = new QFutureWatcher<QPixmap>(this);
        connect(futureWatcher, &QFutureWatcher<QPixmap>::resultReadyAt, this, &ThumbnailWidget::updateThumbnails);
        connect(futureWatcher, &QFutureWatcher<QPixmap>::finished, this, [this]() {
            isLoading = false;
            update();
        });

        futureWatcher->setFuture(future);
    } else {
        // 所有图片都在缓存中，直接更新界面
        emit loadingProgress(loadedCount, totalCount);
        update();
    }
}

QPixmap ThumbnailWidget::loadThumbnail(const QString &path)
{
    QPixmap original;
    if (original.load(path)) {
        QPixmap thumbnail = original.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        // 将缩略图添加到缓存
        QMutexLocker locker(&cacheMutex);
        thumbnailCache.insert(path, thumbnail);

        return thumbnail;
    }
    return QPixmap();
}

void ThumbnailWidget::updateThumbnails()
{
    if (!futureWatcher) return;

    int newLoaded = futureWatcher->future().resultCount();
    if (newLoaded > loadedCount) {
        loadedCount = newLoaded;
        emit loadingProgress(loadedCount, totalCount);

        // 每加载10张更新一次界面，避免频繁刷新
        if (loadedCount % 1 == 0 || loadedCount == totalCount) {
            update();
        }
    }
}

void ThumbnailWidget::stopLoading()
{
    if (futureWatcher && futureWatcher->isRunning()) {
        futureWatcher->cancel();
        futureWatcher->waitForFinished();
    }
    isLoading = false;
}

void ThumbnailWidget::clearThumbnailCache()
{
    QMutexLocker locker(&cacheMutex);
    thumbnailCache.clear();
}

void ThumbnailWidget::setSelectedIndex(int index)
{
    selectedIndex = index;
    update();
    ensureVisible(index);
}

int ThumbnailWidget::getSelectedIndex() const
{
    return selectedIndex;
}

void ThumbnailWidget::ensureVisible(int index)
{
    if (index < 0 || index >= imageList.size()) return;

    int x = thumbnailSpacing;
    int y = thumbnailSpacing;
    int maxWidth = width();

    for (int i = 0; i <= index; ++i) {
        if (i == index) {
            QRect visibleRect(x, y, thumbnailSize.width(), thumbnailSize.height() + 25);
            emit ensureRectVisible(visibleRect);
            break;
        }

        x += thumbnailSize.width() + thumbnailSpacing;
        if (x + thumbnailSize.width() > maxWidth) {
            x = thumbnailSpacing;
            y += thumbnailSize.height() + thumbnailSpacing + 25;
        }
    }
}

void ThumbnailWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), QColor(240, 240, 240));

    if (imageList.isEmpty()) {
        painter.drawText(rect(), Qt::AlignCenter, "没有图片文件\n拖拽图片文件夹到此窗口或右键选择打开文件夹");
        return;
    }

    int x = thumbnailSpacing;
    int y = thumbnailSpacing;
    int maxWidth = width();

    QMutexLocker locker(&cacheMutex);

    for (int i = 0; i < imageList.size(); ++i) {
        QString imagePath = currentDir.absoluteFilePath(imageList.at(i));

        if (thumbnailCache.contains(imagePath)) {
            QPixmap thumbnail = thumbnailCache.value(imagePath);

            int thumbX = x + (thumbnailSize.width() - thumbnail.width()) / 2;
            int thumbY = y + (thumbnailSize.height() - thumbnail.height()) / 2;

            QRect thumbRect(thumbX, thumbY, thumbnail.width(), thumbnail.height());
            QRect borderRect(x, y, thumbnailSize.width(), thumbnailSize.height());

            if (i == selectedIndex) {
                painter.fillRect(borderRect.adjusted(-2, -2, 2, 2), QColor(0, 120, 215));
            }

            painter.drawPixmap(thumbRect, thumbnail);

            QRect textRect(x, y + thumbnailSize.height(), thumbnailSize.width(), 20);
            QString fileName = QFileInfo(imageList.at(i)).fileName();
            painter.setPen(Qt::black);
            painter.drawText(textRect, Qt::AlignCenter | Qt::TextElideMode::ElideRight, fileName);
        } else {
            QRect borderRect(x, y, thumbnailSize.width(), thumbnailSize.height());
            painter.fillRect(borderRect, QColor(200, 200, 200));
            painter.drawText(borderRect, Qt::AlignCenter, "加载中...");
        }

        x += thumbnailSize.width() + thumbnailSpacing;
        if (x + thumbnailSize.width() > maxWidth) {
            x = thumbnailSpacing;
            y += thumbnailSize.height() + thumbnailSpacing + 25;
        }
    }

    if (isLoading) {
        painter.setPen(Qt::blue);
        painter.drawText(10, 20, QString("加载中: %1/%2").arg(loadedCount).arg(totalCount));
    }

    setMinimumHeight(y + thumbnailSize.height() + thumbnailSpacing + 25);
}

void ThumbnailWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selectThumbnailAtPosition(event->pos());
        setFocus(); // 点击时获取焦点，让方向键生效
    } else if (event->button() == Qt::RightButton) {
        // 创建一个新的鼠标事件，使用全局坐标
        QMouseEvent newEvent(event->type(),
                             mapToParent(event->pos()),  // 转换为父窗口坐标
                             event->globalPos(),
                             event->button(),
                             event->buttons(),
                             event->modifiers());

        // 将事件发送给父窗口处理
        QApplication::sendEvent(parentWidget(), &newEvent);
    }
}

void ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        selectThumbnailAtPosition(event->pos());
        if (selectedIndex >= 0) {
            emit thumbnailClicked(selectedIndex);
        }
    }
}

void ThumbnailWidget::selectThumbnailAtPosition(const QPoint &pos)
{
    if (imageList.isEmpty()) return;

    int x = thumbnailSpacing;
    int y = thumbnailSpacing;
    int maxWidth = width();

    for (int i = 0; i < imageList.size(); ++i) {
        QRect clickArea(x, y, thumbnailSize.width(), thumbnailSize.height() + 25);

        if (clickArea.contains(pos)) {
            selectedIndex = i;
            update();
            ensureVisible(i);
            return;
        }

        x += thumbnailSize.width() + thumbnailSpacing;
        if (x + thumbnailSize.width() > maxWidth) {
            x = thumbnailSpacing;
            y += thumbnailSize.height() + thumbnailSpacing + 25;
        }
    }

    selectedIndex = -1;
    update();
}

void ThumbnailWidget::keyPressEvent(QKeyEvent *event)
{
    if (imageList.isEmpty()) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
        QApplication::sendEvent(parentWidget(), event);
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (selectedIndex >= 0) {
            emit thumbnailClicked(selectedIndex);
        }
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
