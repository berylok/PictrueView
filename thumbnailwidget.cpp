#include "thumbnailwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFileInfo>
#include <QApplication>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QThreadPool>
#include <QTimer>
#include <imagewidget.h>

// 初始化静态成员变量
QMap<QString, QPixmap> ThumbnailWidget::thumbnailCache;
QMutex ThumbnailWidget::cacheMutex;

ThumbnailWidget::ThumbnailWidget(QWidget *parent)
    : QWidget(parent),
    thumbnailSize(250, 250),
    thumbnailSpacing(7),
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

// 优化的 setImageList 方法
void ThumbnailWidget::setImageList(const QStringList &list, const QDir &dir)
{
    // 停止之前的加载
    stopLoading();

    imageList = list;
    currentDir = dir;
    selectedIndex = -1;
    loadedCount = 0;
    totalCount = list.size();
    preloadQueue.clear();

    // 立即更新界面显示占位符
    update();

    emit loadingProgress(0, totalCount);

    if (imageList.isEmpty()) {
        return;
    }

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

    // 如果有需要加载的图片，分批加载
    if (!pathsToLoad.isEmpty()) {
        isLoading = true;

        // 第一批立即加载可见区域的图片（前15张）
        int immediateLoadCount = qMin(15, pathsToLoad.size());
        for (int i = 0; i < immediateLoadCount; ++i) {
            QtConcurrent::run([this, path = pathsToLoad[i]]() {
                QPixmap thumbnail = loadThumbnail(path);
                QMetaObject::invokeMethod(this, "onThumbnailLoaded",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, path),
                                          Q_ARG(QPixmap, thumbnail));
            });
        }

        // 剩余的图片延迟加载 - 使用lambda替代单独的方法
        if (pathsToLoad.size() > immediateLoadCount) {
            QStringList remainingPaths = pathsToLoad.mid(immediateLoadCount);
            QTimer::singleShot(100, this, [this, remainingPaths]() {
                // 分批加载剩余图片，每批5张
                const int BATCH_SIZE = 5;
                for (int i = 0; i < remainingPaths.size(); i += BATCH_SIZE) {
                    QStringList batch = remainingPaths.mid(i, qMin(BATCH_SIZE, remainingPaths.size() - i));

                    // 添加延迟，避免同时启动太多线程
                    QTimer::singleShot(i * 50, this, [this, batch]() {
                        for (const QString &path : batch) {
                            QtConcurrent::run([this, path]() {
                                QPixmap thumbnail = loadThumbnail(path);
                                QMetaObject::invokeMethod(this, "onThumbnailLoaded",
                                                          Qt::QueuedConnection,
                                                          Q_ARG(QString, path),
                                                          Q_ARG(QPixmap, thumbnail));
                            });
                        }
                    });
                }
            });
        }
    } else {
        // 所有图片都在缓存中，直接更新界面
        emit loadingProgress(loadedCount, totalCount);
        update();
    }
}

// 修复的缩略图加载完成处理
void ThumbnailWidget::onThumbnailLoaded(const QString &path, const QPixmap &thumbnail)
{
    QMutexLocker locker(&cacheMutex);

    // 再次检查是否已经存在，避免重复添加
    if (!thumbnailCache.contains(path)) {
        thumbnailCache.insert(path, thumbnail);
        loadedCount++;

        // 只在缓存过大时清理，避免频繁清理
        if (thumbnailCache.size() > THUMBNAIL_CACHE_MAX_SIZE * 1.5) {
            cleanupThumbnailCache();
        }

        // 更新进度
        emit loadingProgress(loadedCount, totalCount);

        // 每加载5张或全部加载完成时更新界面
        if (loadedCount % 5 == 0 || loadedCount == totalCount) {
            update();
        }
    }

    // 如果全部加载完成，更新状态
    if (loadedCount >= totalCount) {
        isLoading = false;
        emit loadingProgress(loadedCount, totalCount);
        update();
    }
}

// 修复的缓存清理 - 更保守的策略
void ThumbnailWidget::cleanupThumbnailCache()
{
    // 只在缓存过大时清理
    if (thumbnailCache.size() > THUMBNAIL_CACHE_MAX_SIZE) {
        // 计算需要移除的数量
        int removeCount = thumbnailCache.size() - THUMBNAIL_CACHE_MAX_SIZE;

        // 只移除不在当前列表中的图片
        QList<QString> keysToRemove;
        for (auto it = thumbnailCache.begin(); it != thumbnailCache.end() && removeCount > 0; ++it) {
            QString key = it.key();
            // 检查这个图片是否在当前显示的列表中
            bool inCurrentList = false;
            for (const QString &fileName : imageList) {
                QString imagePath = currentDir.absoluteFilePath(fileName);
                if (imagePath == key) {
                    inCurrentList = true;
                    break;
                }
            }

            // 如果不在当前列表中，可以移除
            if (!inCurrentList) {
                keysToRemove.append(key);
                removeCount--;
            }
        }

        // 移除选定的键
        for (const QString &key : keysToRemove) {
            thumbnailCache.remove(key);
        }

        qDebug() << "清理缩略图缓存，移除了" << keysToRemove.size() << "个项";
    }
}

// 智能预加载
void ThumbnailWidget::schedulePreload(int centerIndex)
{
    if (imageList.isEmpty() || isLoading) return;

    // 清空预加载队列
    preloadQueue.clear();

    // 预加载当前选中项周围的图片
    const int PRELOAD_RANGE = 12; // 增加预加载范围
    int start = qMax(0, centerIndex - PRELOAD_RANGE);
    int end = qMin(imageList.size() - 1, centerIndex + PRELOAD_RANGE);

    // 优先加载靠近中心点的图片
    QList<QString> prioritizedPaths;
    for (int i = centerIndex; i <= end && i <= centerIndex + 3; ++i) {
        QString imagePath = currentDir.absoluteFilePath(imageList.at(i));
        if (!thumbnailCache.contains(imagePath) && !preloadQueue.contains(imagePath)) {
            prioritizedPaths.append(imagePath);
        }
    }
    for (int i = centerIndex - 1; i >= start && i >= centerIndex - 3; --i) {
        QString imagePath = currentDir.absoluteFilePath(imageList.at(i));
        if (!thumbnailCache.contains(imagePath) && !preloadQueue.contains(imagePath)) {
            prioritizedPaths.append(imagePath);
        }
    }

    // 添加其他需要预加载的图片
    for (int i = start; i <= end; ++i) {
        if (qAbs(i - centerIndex) > 3) { // 已经处理过中心附近的图片
            QString imagePath = currentDir.absoluteFilePath(imageList.at(i));
            if (!thumbnailCache.contains(imagePath) && !preloadQueue.contains(imagePath)) {
                prioritizedPaths.append(imagePath);
            }
        }
    }

    // 限制队列大小
    const int preloadQueueCapacity = 20;
    for (const QString &path : prioritizedPaths) {
        if (preloadQueue.size() >= preloadQueueCapacity) break;
        preloadQueue.enqueue(path);
    }

    // 启动预加载
    if (!preloadQueue.isEmpty()) {
        QTimer::singleShot(20, this, &ThumbnailWidget::processPreloadQueue);
    }
}

void ThumbnailWidget::processPreloadQueue()
{
    if (preloadQueue.isEmpty() || isLoading) return;

    // 一次处理3张图片以提高效率
    int processed = 0;
    const int MAX_BATCH = 3;

    while (!preloadQueue.isEmpty() && processed < MAX_BATCH) {
        QString path = preloadQueue.dequeue();
        if (!thumbnailCache.contains(path)) {
            QtConcurrent::run([this, path]() {
                QPixmap thumbnail = loadThumbnail(path);
                QMetaObject::invokeMethod(this, "onThumbnailLoaded",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, path),
                                          Q_ARG(QPixmap, thumbnail));
            });
            processed++;
        }
    }

    // 如果还有更多，继续处理
    if (!preloadQueue.isEmpty()) {
        QTimer::singleShot(30, this, &ThumbnailWidget::processPreloadQueue);
    }
}

// 修复的缩略图加载
QPixmap ThumbnailWidget::loadThumbnail(const QString &path)
{
    QPixmap original;
    if (original.load(path)) {
        // 使用快速缩放算法
        QPixmap thumbnail = original.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return thumbnail;
    }
    return QPixmap();
}

void ThumbnailWidget::stopLoading()
{
    if (futureWatcher && futureWatcher->isRunning()) {
        futureWatcher->cancel();
        futureWatcher->waitForFinished();
    }
    isLoading = false;
    preloadQueue.clear();
}

// 保留原有的其他方法...
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

void ThumbnailWidget::clearThumbnailCache()
{
    QMutexLocker locker(&cacheMutex);
    thumbnailCache.clear();
}

void ThumbnailWidget::setSelectedIndex(int index)
{
    if (index >= -1 && index < imageList.size() && index != selectedIndex) {
        selectedIndex = index;
        update();
        ensureVisible(index);

        // 触发预加载
        schedulePreload(index);

        qDebug() << "ThumbnailWidget 选中索引:" << index;
    }
}

int ThumbnailWidget::getSelectedIndex() const
{
    return selectedIndex;
}

void ThumbnailWidget::ensureVisible(int index)
{
    if (index < 0 || index >= imageList.size()) return;

    int maxWidth = width();
    int itemsPerRow = qMax(1, (maxWidth - thumbnailSpacing) / (thumbnailSize.width() + thumbnailSpacing));

    int row = index / itemsPerRow;
    int col = index % itemsPerRow;

    int x = thumbnailSpacing + col * (thumbnailSize.width() + thumbnailSpacing);
    int y = thumbnailSpacing + row * (thumbnailSize.height() + thumbnailSpacing + 25);

    // 创建一个稍大的矩形区域，确保缩略图完全可见
    QRect visibleRect(x, y, thumbnailSize.width(), thumbnailSize.height() + 25);
    emit ensureRectVisible(visibleRect);
}

void ThumbnailWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    if (imageList.isEmpty()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,tr("欢迎使用图片查看器！\n\n"
                                                     "使用方法：\n"
                                                     "• 按 Ctrl+O 打开文件夹浏览图片\n"
                                                     "• 按 Ctrl+Shift+O 打开单张图片\n"
                                                     "• 按 F1 查看详细使用说明\n\n"
                                                     "祝您使用愉快！"
                                                     "\n"
                                                     "\n"
                                                     "没有图片文件\n拖拽图片文件夹到此窗口或右键选择打开文件夹"
                                                     "\n"
                                                     "\n"
                                                     "F1 查看帮助 或右键弹出菜单使用"));
        return;
    }

    int x = thumbnailSpacing;
    int y = thumbnailSpacing;
    int maxWidth = width();
    int itemsPerRow = qMax(1, (maxWidth - thumbnailSpacing) / (thumbnailSize.width() + thumbnailSpacing));

    QMutexLocker locker(&cacheMutex);

    for (int i = 0; i < imageList.size(); ++i) {
        QString imagePath = currentDir.absoluteFilePath(imageList.at(i));

        // 计算当前缩略图的位置
        int row = i / itemsPerRow;
        int col = i % itemsPerRow;
        int currentX = thumbnailSpacing + col * (thumbnailSize.width() + thumbnailSpacing);
        int currentY = thumbnailSpacing + row * (thumbnailSize.height() + thumbnailSpacing + 25);

        if (thumbnailCache.contains(imagePath)) {
            QPixmap thumbnail = thumbnailCache.value(imagePath);

            int thumbX = currentX + (thumbnailSize.width() - thumbnail.width()) / 2;
            int thumbY = currentY + (thumbnailSize.height() - thumbnail.height()) / 2;

            QRect thumbRect(thumbX, thumbY, thumbnail.width(), thumbnail.height());
            QRect borderRect(currentX, currentY, thumbnailSize.width(), thumbnailSize.height());

            if (i == selectedIndex) {
                // 绘制选中状态
                painter.fillRect(borderRect.adjusted(-2, -2, 2, 2), QColor(0, 120, 215));
            }

            painter.drawPixmap(thumbRect, thumbnail);

            QRect textRect(currentX, currentY + thumbnailSize.height(), thumbnailSize.width(), 20);
            QString fileName = QFileInfo(imageList.at(i)).fileName();
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter | Qt::TextElideMode::ElideRight, fileName);
        } else {
            QRect borderRect(currentX, currentY, thumbnailSize.width(), thumbnailSize.height());
            painter.fillRect(borderRect, QColor(50, 50, 50));
            painter.setPen(Qt::white);
            painter.drawText(borderRect, Qt::AlignCenter, "加载中...");
        }
    }

    if (isLoading) {
        painter.setPen(Qt::white);
        painter.drawText(10, 20, QString("加载中: %1/%2").arg(loadedCount).arg(totalCount));
    }

    // 计算最小高度
    int rows = (imageList.size() + itemsPerRow - 1) / itemsPerRow;
    int minHeight = thumbnailSpacing + rows * (thumbnailSize.height() + thumbnailSpacing + 25);
    setMinimumHeight(minHeight);
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
            // 重要：立即发射信号，不经过任何中间状态
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
    qDebug() << "ThumbnailWidget 接收到按键:" << event->key();

    if (imageList.isEmpty()) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
        // 左右方向键：在缩略图部件中直接处理选中切换
        {
            int newIndex = selectedIndex;
            if (newIndex < 0) newIndex = 0; // 如果没有选中，从第一个开始

            if (event->key() == Qt::Key_Left) {
                newIndex = (newIndex - 1 + imageList.size()) % imageList.size();
            } else {
                newIndex = (newIndex + 1) % imageList.size();
            }

            if (newIndex != selectedIndex) {
                selectedIndex = newIndex;
                update(); // 重绘以显示新的选中状态
                ensureVisible(selectedIndex); // 确保选中的缩略图可见

                // 通知父窗口当前选中项已更改
                QMetaObject::invokeMethod(parentWidget(), "onThumbnailSelectionChanged",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, selectedIndex));
            }
            event->accept();
        }
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
        // 上下方向键：转发给父窗口处理滚动
        QApplication::sendEvent(parentWidget(), event);
        event->accept();
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Home:
    case Qt::Key_End:
        // 将导航键转发给父窗口处理
        QApplication::sendEvent(parentWidget(), event);
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        qDebug() << "处理回车键，选中索引:" << selectedIndex;
        if (selectedIndex >= 0) {
            emit thumbnailClicked(selectedIndex);
        }
        event->accept();
        break;

    default:
        QWidget::keyPressEvent(event);
    }
}

void ThumbnailWidget::clearThumbnailCacheForImage(const QString &imagePath)
{
    QMutexLocker locker(&cacheMutex);
    thumbnailCache.remove(imagePath);
}
