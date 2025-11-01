#include "thumbnailwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFileInfo>
#include <QApplication>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <imagewidget.h>
#include <QPainterPath>

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
    setFocusPolicy(Qt::StrongFocus); // 确保能够接收键盘事件
}

ThumbnailWidget::~ThumbnailWidget()
{
    stopLoading();
}

bool ThumbnailWidget::isArchiveFile(const QString &fileName) const
{
    QString lowerName = fileName.toLower();
    return (lowerName.endsWith(".zip") || lowerName.endsWith(".rar") ||
            lowerName.endsWith(".7z") || lowerName.endsWith(".tar") ||
            lowerName.endsWith(".gz") || lowerName.endsWith(".bz2"));
}

QPixmap ThumbnailWidget::createArchiveIcon() const
{
    QPixmap icon(thumbnailSize);
    icon.fill(QColor(70, 130, 180)); // 钢蓝色背景

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制压缩包图标
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(QColor(100, 160, 210));

    // 绘制文件夹形状
    QPainterPath path;
    path.moveTo(20, 40);
    path.lineTo(60, 40);
    path.lineTo(70, 60);
    path.lineTo(30, 60);
    path.closeSubpath();

    painter.drawPath(path);

    // 绘制拉链
    painter.setPen(QPen(Qt::white, 3));
    painter.drawLine(45, 40, 45, 60);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(icon.rect(), Qt::AlignCenter, "ZIP");

    return icon;
}

void ThumbnailWidget::setImageList(const QStringList &list, const QDir &dir)
{
    qDebug() << "设置缩略图列表，数量:" << list.size();

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
        for (const QString &filePath : imageList) {
            QString cacheKey;
            if (filePath.contains("|")) {
                // 压缩包文件：直接使用完整路径作为缓存键
                cacheKey = filePath;
            } else {
                // 普通文件：使用绝对路径作为缓存键
                cacheKey = currentDir.absoluteFilePath(filePath);
            }

            if (!thumbnailCache.contains(cacheKey)) {
                pathsToLoad.append(filePath);
                qDebug() << "需要加载缩略图:" << filePath << "缓存键:" << cacheKey;
            } else {
                loadedCount++;
                qDebug() << "缩略图已在缓存中:" << filePath << "缓存键:" << cacheKey;
            }
        }
    }

    // 如果有需要加载的图片
    if (!pathsToLoad.isEmpty()) {
        isLoading = true;
        qDebug() << "开始异步加载" << pathsToLoad.size() << "个缩略图";

        // 使用 QtConcurrent 异步加载缩略图
        QFuture<QPixmap> future =
            QtConcurrent::mapped(pathsToLoad, [this](const QString &path) {
            return loadThumbnail(path);
        });

        // 创建监视器来跟踪进度
        futureWatcher = new QFutureWatcher<QPixmap>(this);
        connect(futureWatcher, &QFutureWatcher<QPixmap>::resultReadyAt, this,
                &ThumbnailWidget::updateThumbnails);
        connect(futureWatcher, &QFutureWatcher<QPixmap>::finished, this,
                [this]() {
                    isLoading = false;
            qDebug() << "缩略图加载完成，总共加载:" << loadedCount << "/"
                             << totalCount;
                    update(); // 确保最后更新一次界面
        });

        futureWatcher->setFuture(future);
    } else {
        // 所有图片都在缓存中，直接更新界面
        emit loadingProgress(loadedCount, totalCount);
        update();
        qDebug() << "所有缩略图已在缓存中，直接更新界面";
    }
}

QPixmap ThumbnailWidget::loadThumbnail(const QString &path)
{
    qDebug() << "开始加载缩略图:" << path;

    // 检查是否是压缩包内的文件（路径包含"|"分隔符）
    if (path.contains("|")) {
        qDebug() << "检测到压缩包内文件路径:" << path;

        // 压缩包内的文件，需要从主窗口获取
        ImageWidget *parentWidget = qobject_cast<ImageWidget*>(this->parent());
        if (parentWidget) {
            // 直接调用父窗口的方法获取缩略图
            QPixmap thumbnail = parentWidget->getArchiveThumbnail(path);
            if (!thumbnail.isNull()) {
                qDebug() << "成功获取压缩包缩略图，尺寸:" << thumbnail.size();
                QMutexLocker locker(&cacheMutex);
                thumbnailCache.insert(path, thumbnail);
                return thumbnail;
            } else {
                qDebug() << "获取压缩包缩略图失败，返回空缩略图";
            }
        } else {
            qDebug() << "无法获取父窗口，无法加载压缩包缩略图";
        }
        return QPixmap();
    }

    // 普通文件加载逻辑
    QPixmap original;

    // 构建完整文件路径
    QString fullPath = currentDir.absoluteFilePath(path);
    qDebug() << "加载普通文件缩略图，完整路径:" << fullPath;

    // 首先检查文件是否存在
    QFileInfo fileInfo(fullPath);
    if (!fileInfo.exists()) {
        qDebug() << "文件不存在:" << fullPath;
        return QPixmap();
    }

    if (original.load(fullPath)) {
        QPixmap thumbnail = original.scaled(thumbnailSize, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);

        qDebug() << "成功加载普通文件缩略图，路径:" << fullPath
                 << "原始尺寸:" << original.size()
                 << "缩略图尺寸:" << thumbnail.size();

        QMutexLocker locker(&cacheMutex);
        thumbnailCache.insert(fullPath, thumbnail); // 使用完整路径作为缓存键

        return thumbnail;
    }

    qDebug() << "加载缩略图失败:" << fullPath;
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

//确保设置选中索引时自动滚动到可见区域：
void ThumbnailWidget::setSelectedIndex(int index)
{
    if (index >= -1 && index < imageList.size() && index != selectedIndex) {
        selectedIndex = index;
        update(); // 确保这里调用了 update() 来触发重绘
        ensureVisible(index);// 确保选中的缩略图可见

        qDebug() << "ThumbnailWidget 选中索引:" << index;
    }
}

int ThumbnailWidget::getSelectedIndex() const
{
    return selectedIndex;
}

//确保这个方法能正确计算缩略图位置并发出信号：
void ThumbnailWidget::ensureVisible(int index)
{
    if (index < 0 || index >= imageList.size()) return;

    int maxWidth = width();
    int itemsPerRow = qMax(1, (maxWidth - thumbnailSpacing) /
                                  (thumbnailSize.width() + thumbnailSpacing));

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
    int itemsPerRow = qMax(1, (maxWidth - thumbnailSpacing) /
                                  (thumbnailSize.width() + thumbnailSpacing));

    QMutexLocker locker(&cacheMutex);

    for (int i = 0; i < imageList.size(); ++i) {
        QString fileName = imageList.at(i);

        // 统一构建缓存键的逻辑
        QString cacheKey;
        if (fileName.contains("|")) {
            // 压缩包文件：直接使用完整路径作为缓存键
            cacheKey = fileName;
        } else {
            // 普通文件：使用绝对路径作为缓存键
            cacheKey = currentDir.absoluteFilePath(fileName);
        }

        // 计算当前缩略图的位置
        int row = i / itemsPerRow;
        int col = i % itemsPerRow;
        int currentX =
            thumbnailSpacing + col * (thumbnailSize.width() + thumbnailSpacing);
        int currentY = thumbnailSpacing +
                       row * (thumbnailSize.height() + thumbnailSpacing + 25);

        // 检查是否是压缩包文件
        if (isArchiveFile(fileName) && !fileName.contains("|")) {
            // 绘制压缩包缩略图（仅对顶级压缩包文件）
            QRect borderRect(currentX, currentY, thumbnailSize.width(),
                             thumbnailSize.height());

            if (i == selectedIndex) {
                painter.fillRect(borderRect.adjusted(-2, -2, 2, 2),
                                 QColor(0, 120, 215));
            }

            QPixmap archiveIcon = createArchiveIcon();
            painter.drawPixmap(borderRect, archiveIcon);

            QRect textRect(currentX, currentY + thumbnailSize.height(),
                           thumbnailSize.width(), 20);
            painter.setPen(Qt::white);
            painter.drawText(textRect,
                             Qt::AlignCenter | Qt::TextElideMode::ElideRight,
                             fileName);

        } else if (thumbnailCache.contains(cacheKey)) {
            // 普通图片或压缩包内图片的原有逻辑
            QPixmap thumbnail = thumbnailCache.value(cacheKey);

            int thumbX =
                currentX + (thumbnailSize.width() - thumbnail.width()) / 2;
            int thumbY =
                currentY + (thumbnailSize.height() - thumbnail.height()) / 2;

            QRect thumbRect(thumbX, thumbY, thumbnail.width(),
                            thumbnail.height());
            QRect borderRect(currentX, currentY, thumbnailSize.width(),
                             thumbnailSize.height());

            if (i == selectedIndex) {
                painter.fillRect(borderRect.adjusted(-2, -2, 2, 2),
                                 QColor(0, 120, 215));
            }

            painter.drawPixmap(thumbRect, thumbnail);

            QRect textRect(currentX, currentY + thumbnailSize.height(),
                           thumbnailSize.width(), 20);
            QString displayName = QFileInfo(fileName).fileName();
            painter.setPen(Qt::white);
            painter.drawText(textRect,
                             Qt::AlignCenter | Qt::TextElideMode::ElideRight,
                             displayName);
        } else {
            // 加载中的图片
            QRect borderRect(currentX, currentY, thumbnailSize.width(),
                             thumbnailSize.height());
            painter.fillRect(borderRect, QColor(50, 50, 50));
            painter.setPen(Qt::white);
            painter.drawText(borderRect, Qt::AlignCenter, tr("加载中..."));

            QRect textRect(currentX, currentY + thumbnailSize.height(),
                           thumbnailSize.width(), 20);
            QString displayName = QFileInfo(fileName).fileName();
            painter.drawText(textRect,
                             Qt::AlignCenter | Qt::TextElideMode::ElideRight,
                             displayName);
        }
    }

    if (isLoading) {
        painter.setPen(Qt::white);
        painter.drawText(
            10, 20, QString("加载中: %1/%2").arg(loadedCount).arg(totalCount));
    }

    // 计算最小高度
    int rows = (imageList.size() + itemsPerRow - 1) / itemsPerRow;
    int minHeight = thumbnailSpacing +
                    rows * (thumbnailSize.height() + thumbnailSpacing + 25);
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

