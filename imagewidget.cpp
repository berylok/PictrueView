#include "imagewidget.h"
#include "qapplication.h"
#include <QPainter>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QFileDialog>
#include <QClipboard>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyle>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <windows.h>

// imagewidget.cpp
ImageWidget::ImageWidget(QWidget *parent) : QWidget(parent),
    scaleFactor(1.0),
    isDraggingWindow(false),
    isPanningImage(false),
    currentImageIndex(-1),
    isSlideshowActive(false),
    slideshowInterval(3000),
    currentViewMode(ThumbnailView),
    thumbnailSize(150, 150),
    thumbnailSpacing(10),
    panOffset(0, 0),
    currentViewStateType(FitToWindow) // 修改为合适大小
{

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建滚动区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);

    // 创建缩略图部件
    thumbnailWidget = new ThumbnailWidget(this);
    scrollArea->setWidget(thumbnailWidget);

    // 连接信号
    // 修改信号连接 - 确保双击能打开图片
    connect(thumbnailWidget, &ThumbnailWidget::thumbnailClicked, this, &ImageWidget::onThumbnailClicked);
    mainLayout->addWidget(scrollArea);

    // 启用拖拽功能
    setAcceptDrops(true);

    // 设置窗口无边框
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    // 初始化窗口大小为最大化
    showMaximized();
    setFocusPolicy(Qt::StrongFocus);

    // 创建幻灯定时器
    slideshowTimer = new QTimer(this);
    connect(slideshowTimer, &QTimer::timeout, this, &ImageWidget::slideshowNext);
}

ImageWidget::~ImageWidget()
{
    delete slideshowTimer;
    delete thumbnailWidget;
    delete scrollArea;
}

// imagewidget.cpp
void ImageWidget::setCurrentDir(const QDir &dir)
{
    currentDir = dir;
}

// imagewidget.cpp
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
    logMessage("图片已调整到合适大小");
}

void ImageWidget::actualSize()
{
    if (pixmap.isNull()) return;

    scaleFactor = 1.0;
    panOffset = QPointF(0, 0);
    currentViewStateType = ActualSize; // 设置为实际大小模式

    update();
    logMessage("图片已调整到实际大小");
}

// imagewidget.cpp
bool ImageWidget::loadImage(const QString &filePath, bool fromCache)
{
    // 保存当前图片的视图状态（如果是手动调整的）
    if (!currentImagePath.isEmpty() && !pixmap.isNull() && currentViewStateType == ManualAdjustment) {
        // 如果是手动调整的，保存状态以便后续图片使用
        qDebug() << "保存手动调整状态: 缩放=" << scaleFactor << ", 偏移=" << panOffset;
    }

    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        return false;
    }

    if (fromCache && imageCache.contains(filePath)) {
        pixmap = imageCache.value(filePath);
    }
    else if (pixmap.load(filePath)) {
        if (!fromCache) {
            imageCache.insert(filePath, pixmap);
        }
    } else {
        return false;
    }

    // 根据当前视图状态类型决定如何设置新图片的视图状态
    switch (currentViewStateType) {
    case FitToWindow:
        // 使用合适大小的设置
        {
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
        }
        break;
    case ActualSize:
        // 使用实际大小的设置
        scaleFactor = 1.0;
        panOffset = QPointF(0, 0);
        break;
    case ManualAdjustment:
        // 保持当前的缩放和偏移（手动调整的状态）
        // 不需要做任何改变
        break;
    }

    currentImagePath = filePath;

    // 检查目录是否改变
    bool dirChanged = (currentDir != fileInfo.absoluteDir());
    if (dirChanged) {
        currentDir = fileInfo.absoluteDir();
        loadImageList();
    }

    // 确保当前图片索引正确设置
    currentImageIndex = imageList.indexOf(fileInfo.fileName());

    // 如果当前是缩略图模式，更新选中项
    if (currentViewMode == ThumbnailView && currentImageIndex >= 0) {
        thumbnailWidget->setSelectedIndex(currentImageIndex);
    }

    update();
    updateWindowTitle();

    return true;
}

// imagewidget.cpp
void ImageWidget::loadImageList()
{
    QStringList newImageList;

    QFileInfoList fileList = currentDir.entryInfoList(QDir::Files);
    QStringList imageFilters = {"*.png", "*.jpg", "*.bmp", "*.jpeg", "*.webp", "*.gif", "*.tiff", "*.tif"};

    foreach (const QFileInfo &fileInfo, fileList) {
        foreach (const QString &filter, imageFilters) {
            if (fileInfo.fileName().endsWith(filter.mid(1), Qt::CaseInsensitive)) {
                newImageList.append(fileInfo.fileName());
                break;
            }
        }
    }

    newImageList.sort();

    // 只有当图片列表实际发生变化时才更新和输出日志
    if (newImageList != imageList) {
        imageList = newImageList;
        thumbnailWidget->setImageList(imageList, currentDir);
        qDebug() << "找到图片文件:" << imageList.size() << "个";
    }
}

// imagewidget.cpp
bool ImageWidget::loadImageByIndex(int index, bool fromCache)
{
    if (imageList.isEmpty() || index < 0 || index >= imageList.size()) {
        return false;
    }

    QString imagePath = currentDir.absoluteFilePath(imageList.at(index));

    // 直接调用 loadImage，它会根据 currentViewStateType 设置正确的视图状态
    bool result = loadImage(imagePath, fromCache);

    // 更新当前索引
    if (result) {
        currentImageIndex = index;

        // 如果当前是缩略图模式，更新选中项
        if (currentViewMode == ThumbnailView) {
            thumbnailWidget->setSelectedIndex(currentImageIndex);
        }

        // 预加载下一张图片（用于幻灯片）
        if (isSlideshowActive) {
            int nextIndex = (currentImageIndex + 1) % imageList.size();
            QString nextPath = currentDir.absoluteFilePath(imageList.at(nextIndex));

            if (!imageCache.contains(nextPath)) {
                QtConcurrent::run([this, nextPath]() {
                    QPixmap tempPixmap;
                    if (tempPixmap.load(nextPath)) {
                        QMutexLocker locker(&cacheMutex);
                        imageCache.insert(nextPath, tempPixmap);
                    }
                });
            }
        }
    }

    return result;
}

void ImageWidget::loadNextImage()
{
    if (imageList.isEmpty()) return;
    int nextIndex = (currentImageIndex + 1) % imageList.size();
    loadImageByIndex(nextIndex, true);
}

void ImageWidget::loadPreviousImage()
{
    if (imageList.isEmpty()) return;
    int prevIndex = (currentImageIndex - 1 + imageList.size()) % imageList.size();
    loadImageByIndex(prevIndex, true);
}

void ImageWidget::preloadAllImages()
{
    imageCache.clear();

    int loadedCount = 0;
    for (const QString &fileName : imageList) {
        QString filePath = currentDir.absoluteFilePath(fileName);
        QPixmap tempPixmap;
        if (tempPixmap.load(filePath)) {
            imageCache.insert(filePath, tempPixmap);
            loadedCount++;
            // 移除单条日志消息，减少干扰
        }
    }
    // 只保留一条总结性日志
    logMessage("预加载完成: " + QString::number(loadedCount) + "/" +
               QString::number(imageList.size()) + " 张图片");

    // 移除消息框提示，减少干扰
    updateWindowTitle();
}

void ImageWidget::clearImageCache()
{
    int cacheSize = imageCache.size();
    imageCache.clear();
    logMessage("清空内存缓存: 释放了 " + QString::number(cacheSize) + " 张图片");
    updateWindowTitle();
}

void ImageWidget::startSlideshow()
{
    if (imageList.size() > 1) {
        // 不再预加载所有图片，改为按需加载
        isSlideshowActive = true;
        slideshowTimer->start(slideshowInterval);
        updateWindowTitle();
        // 移除日志消息，避免干扰
    }
}

void ImageWidget::stopSlideshow()
{
    isSlideshowActive = false;
    slideshowTimer->stop();
    updateWindowTitle();
    logMessage("幻灯:停止");
}

void ImageWidget::toggleSlideshow()
{
    if (isSlideshowActive) {
        stopSlideshow();
    } else {
        startSlideshow();
    }
}

// imagewidget.cpp
void ImageWidget::setSlideshowInterval(int interval)
{
    slideshowInterval = interval;
    if (isSlideshowActive) {
        slideshowTimer->setInterval(slideshowInterval);
    }
    logMessage(QString("幻灯间隔:设置为 %1 毫秒").arg(interval));
}

void ImageWidget::slideshowNext()
{
    if (imageList.isEmpty()) {
        stopSlideshow();
        return;
    }

    int nextIndex = (currentImageIndex + 1) % imageList.size();

    // 预加载下一张图片（如果不在缓存中）
    if (!imageCache.contains(currentDir.absoluteFilePath(imageList.at(nextIndex)))) {
        QtConcurrent::run([this, nextIndex]() {
            QString nextPath = currentDir.absoluteFilePath(imageList.at(nextIndex));
            QPixmap tempPixmap;
            if (tempPixmap.load(nextPath)) {
                QMutexLocker locker(&cacheMutex);
                imageCache.insert(nextPath, tempPixmap);
            }
        });
    }

    // 加载当前图片
    loadImageByIndex(nextIndex, true);
}

void ImageWidget::updateWindowTitle()
{
    QString title;

    if (!currentImagePath.isEmpty()) {
        QFileInfo fileInfo(currentImagePath);
        title = QString("%1 (%2/%3) - %4模式")
                    .arg(fileInfo.fileName())
                    .arg(currentImageIndex + 1)
                    .arg(imageList.size())
                    .arg(currentViewMode == SingleView ? "单张" : "缩略图");
    } else if (!pixmap.isNull()) {
        title = "剪贴板图片";
    } else {
        title = "图片查看器 - 缩略图模式";
    }

    // 移除缓存信息显示，减少标题栏长度
    if (isSlideshowActive) {
        title += " [幻灯中]";
    }

    setWindowTitle(title);
}

QString ImageWidget::getShortPathName(const QString &longPath)
{
    wchar_t shortPath[MAX_PATH];
    if (GetShortPathName(longPath.toStdWString().c_str(), shortPath, MAX_PATH) > 0) {
        return QString::fromWCharArray(shortPath);
    }
    return longPath;
}

void ImageWidget::logMessage(const QString &message)
{
    // 注释掉日志记录，减少干扰
    /*
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/app_log.txt";
    QFile logFile(logPath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << " - " << message << "\n";
        logFile.close();
    }
    */
}

void ImageWidget::registerFileAssociation(const QString &fileExtension, const QString &fileTypeName, const QString &openCommand)
{
    QString keyName = QString(".%1").arg(fileExtension);
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, reinterpret_cast<const wchar_t*>(keyName.utf16()), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(fileTypeName.utf16()), (fileTypeName.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    keyName = QString("%1").arg(fileTypeName);
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, reinterpret_cast<const wchar_t*>(keyName.utf16()), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(fileTypeName.utf16()), (fileTypeName.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    keyName = QString("%1\\shell\\open\\command").arg(fileTypeName);
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, reinterpret_cast<const wchar_t*>(keyName.utf16()), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(openCommand.utf16()), (openCommand.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }
}

// imagewidget.cpp
void ImageWidget::switchToThumbnailView()
{
    // 不再保存当前状态
    currentViewMode = ThumbnailView;
    scrollArea->show();

    // 设置当前选中的缩略图索引
    if (currentImageIndex >= 0 && currentImageIndex < imageList.size()) {
        thumbnailWidget->setSelectedIndex(currentImageIndex);
    } else if (!imageList.isEmpty()) {
        // 如果没有当前选中项，但图片列表不为空，选择第一项
        thumbnailWidget->setSelectedIndex(0);
    }

    update();
}

void ImageWidget::switchToSingleView(int index)
{
    currentViewMode = SingleView;
    scrollArea->hide();

    // 如果有有效的索引，加载对应的图片
    if (index >= 0 && index < imageList.size()) {
        // 不再保存当前状态
        loadImageByIndex(index);
    }

    update();
}

// imagewidget.cpp
void ImageWidget::openFolder()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "选择图片文件夹",
                                                           QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    if (!folderPath.isEmpty()) {
        // 重置为合适大小模式
        currentViewStateType = FitToWindow;

        currentDir = QDir(folderPath);
        loadImageList();
        currentViewMode = ThumbnailView;
        currentImageIndex = -1;
        scrollArea->show();
        update();
    }
}


void ImageWidget::paintEvent(QPaintEvent *event)
{
    if (currentViewMode == SingleView) {
        Q_UNUSED(event);
        QPainter painter(this);

        if (!testAttribute(Qt::WA_TranslucentBackground)) {
            painter.fillRect(rect(), Qt::black);
        }

        if (pixmap.isNull()) {
            painter.drawText(rect(), Qt::AlignCenter, "没有图片");
            return;
        }

        QSize scaledSize = pixmap.size() * scaleFactor;
        QPixmap scaledPixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPointF offset((width() - scaledPixmap.width()) / 2 + panOffset.x(),
                       (height() - scaledPixmap.height()) / 2 + panOffset.y());

        painter.drawPixmap(offset, scaledPixmap);
    }
}

void ImageWidget::onThumbnailClicked(int index)
{
    currentImageIndex = index;
    switchToSingleView(index); // 切换到单张视图
}

void ImageWidget::onEnsureRectVisible(const QRect &rect)
{
    scrollArea->ensureVisible(rect.x(), rect.y(), rect.width(), rect.height());
}

void ImageWidget::navigateThumbnails(int key)
{
    if (imageList.isEmpty()) return;

    int columns = (width() - thumbnailSpacing) / (thumbnailSize.width() + thumbnailSpacing);
    if (columns <= 0) columns = 1;

    int newIndex = currentImageIndex;
    if (newIndex < 0) newIndex = 0;

    switch (key) {
    case Qt::Key_Left:
        newIndex = (newIndex - 1 + imageList.size()) % imageList.size();
        break;
    case Qt::Key_Right:
        newIndex = (newIndex + 1) % imageList.size();
        break;
    case Qt::Key_Up:
        newIndex = (newIndex - columns + imageList.size()) % imageList.size();
        break;
    case Qt::Key_Down:
        newIndex = (newIndex + columns) % imageList.size();
        break;
    }

    currentImageIndex = newIndex;
    thumbnailWidget->setSelectedIndex(newIndex);

    // 更新窗口标题显示当前选中位置
    updateWindowTitle();
}

void ImageWidget::toggleTitleBar()
{
    QRect currentGeometry = geometry();
    int titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);

    if (windowFlags() & Qt::FramelessWindowHint) {
        setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
        setGeometry(currentGeometry.x(), currentGeometry.y() + titleBarHeight,
                    currentGeometry.width(), currentGeometry.height() - titleBarHeight);
    } else {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        setGeometry(currentGeometry.x(), currentGeometry.y() - titleBarHeight,
                    currentGeometry.width(), currentGeometry.height() + titleBarHeight);
    }
    show();
}

void ImageWidget::toggleAlwaysOnTop()
{
    if (windowFlags() & Qt::WindowStaysOnTopHint) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    show();
}

void ImageWidget::toggleTransparentBackground()
{
    if (testAttribute(Qt::WA_TranslucentBackground)) {
        setAttribute(Qt::WA_TranslucentBackground, false);
    } else {
        setAttribute(Qt::WA_TranslucentBackground, true);
    }
    update();
}

void ImageWidget::copyImageToClipboard()
{
    if (!pixmap.isNull()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setPixmap(pixmap);
        logMessage("图片已复制到剪贴板");
    }
}

void ImageWidget::pasteImageFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage()) {
        QImage image = clipboard->image();
        if (!image.isNull()) {
            pixmap = QPixmap::fromImage(image);
            scaleFactor = 1.0;
            panOffset = QPointF(0, 0);
            currentImagePath.clear();
            currentImageIndex = -1;
            imageList.clear();
            update();
            updateWindowTitle();
            logMessage("从剪贴板粘贴图片");
        }
    }
}

void ImageWidget::saveImage()
{
    if (pixmap.isNull()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "保存图片",
                                                    QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                    "Images (*.png *.jpg *.bmp *.jpeg *.webp)");
    if (!fileName.isEmpty()) {
        if (pixmap.save(fileName)) {
            logMessage("图片已保存: " + fileName);
        } else {
            logMessage("保存图片失败: " + fileName);
        }
    }
}

void ImageWidget::openImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开图片",
                                                    QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                    "Images (*.png *.jpg *.bmp *.jpeg *.webp *.gif *.tiff *.tif)");
    if (!fileName.isEmpty()) {
        loadImage(fileName);
        switchToSingleView();
    }
}

// imagewidget.cpp
void ImageWidget::showContextMenu(const QPoint &globalPos)
{
    QMenu contextMenu(this);

    if (currentViewMode == SingleView) {
        contextMenu.addAction("返回缩略图模式", this, &ImageWidget::switchToThumbnailView);
        contextMenu.addSeparator();

        // 添加合适大小和实际大小的菜单项
        contextMenu.addAction("合适大小 (↑)", this, &ImageWidget::fitToWindow);
        contextMenu.addAction("实际大小 (↓)", this, &ImageWidget::actualSize);
        contextMenu.addSeparator();
    }

    contextMenu.addAction("打开文件夹", this, &ImageWidget::openFolder);
    contextMenu.addSeparator();

    contextMenu.addAction("打开图片", this, &ImageWidget::openImage);
    contextMenu.addAction("保存图片", this, &ImageWidget::saveImage);
    contextMenu.addSeparator();
    contextMenu.addAction("复制图片", this, &ImageWidget::copyImageToClipboard);
    contextMenu.addAction("粘贴图片", this, &ImageWidget::pasteImageFromClipboard);
    contextMenu.addSeparator();

    contextMenu.addAction("切换标题栏", this, &ImageWidget::toggleTitleBar);
    contextMenu.addAction("切换置顶", this, &ImageWidget::toggleAlwaysOnTop);
    contextMenu.addAction("切换透明背景", this, &ImageWidget::toggleTransparentBackground);
    contextMenu.addSeparator();

    QMenu *slideshowMenu = contextMenu.addMenu("幻灯功能");
    QAction *slideshowAction = slideshowMenu->addAction(isSlideshowActive ? "停止幻灯" : "开始幻灯");
    connect(slideshowAction, &QAction::triggered, this, &ImageWidget::toggleSlideshow);

    slideshowMenu->addSeparator();

    QMenu *intervalMenu = slideshowMenu->addMenu("切换间隔");
    QAction *interval1s = intervalMenu->addAction("1秒");
    QAction *interval2s = intervalMenu->addAction("2秒");
    QAction *interval3s = intervalMenu->addAction("3秒");
    QAction *interval5s = intervalMenu->addAction("5秒");
    QAction *interval10s = intervalMenu->addAction("10秒");

    interval1s->setCheckable(true);
    interval2s->setCheckable(true);
    interval3s->setCheckable(true);
    interval5s->setCheckable(true);
    interval10s->setCheckable(true);

    interval1s->setChecked(slideshowInterval == 1000);
    interval2s->setChecked(slideshowInterval == 2000);
    interval3s->setChecked(slideshowInterval == 3000);
    interval5s->setChecked(slideshowInterval == 5000);
    interval10s->setChecked(slideshowInterval == 10000);

    connect(interval1s, &QAction::triggered, [this]() { setSlideshowInterval(1000); });
    connect(interval2s, &QAction::triggered, [this]() { setSlideshowInterval(2000); });
    connect(interval3s, &QAction::triggered, [this]() { setSlideshowInterval(3000); });
    connect(interval5s, &QAction::triggered, [this]() { setSlideshowInterval(5000); });
    connect(interval10s, &QAction::triggered, [this]() { setSlideshowInterval(10000); });

    contextMenu.addSeparator();
    contextMenu.addAction("退出", this, &QWidget::close);

    contextMenu.exec(globalPos);
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (currentViewMode == ThumbnailView) {
        // 在缩略图模式下，只处理右键点击（显示菜单）
        if (event->button() == Qt::RightButton) {
            showContextMenu(event->globalPos());
        }
        // 左键和中键事件交给 ThumbnailWidget 处理
        else {
            QWidget::mousePressEvent(event);
        }
    } else {
        // 单张模式下的原有处理
        if (event->button() == Qt::MiddleButton) {
            isDraggingWindow = true;
            dragStartPosition = event->globalPos() - frameGeometry().topLeft();
        } else if (event->button() == Qt::RightButton) {
            showContextMenu(event->globalPos());
        } else if (event->button() == Qt::LeftButton) {
            isPanningImage = true;
            panStartPosition = event->pos();
        }
    }
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isDraggingWindow && (event->buttons() & Qt::MiddleButton)) {
        QPoint newPosition = event->globalPos() - dragStartPosition;
        move(newPosition);
    } else if (isPanningImage && (event->buttons() & Qt::LeftButton)) {
        // 标记为手动调整
        currentViewStateType = ManualAdjustment;

        QPointF delta = event->pos() - panStartPosition;
        panOffset += delta;
        panStartPosition = event->pos();
        update();
    }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        isDraggingWindow = false;
    } else if (event->button() == Qt::LeftButton) {
        isPanningImage = false;
    }
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (currentViewMode == ThumbnailView && thumbnailWidget->getSelectedIndex() >= 0) {
            switchToSingleView(thumbnailWidget->getSelectedIndex());
        } else if (currentViewMode == SingleView) {
            switchToThumbnailView();
        }
    }
}

// imagewidget.cpp
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
        QPointF imagePos = (mousePos - QPointF(width() / 2, height() / 2) - panOffset) / oldScaleFactor;
        panOffset = mousePos - QPointF(width() / 2, height() / 2) - imagePos * scaleFactor;

        update();
    }
}

void ImageWidget::keyPressEvent(QKeyEvent *event)
{
    if (currentViewMode == SingleView) {
        switch (event->key()) {
        case Qt::Key_Left:
            loadPreviousImage();
            break;
        case Qt::Key_Right:
            loadNextImage();
            break;
        case Qt::Key_Up: // 上箭头 - 合适大小
            fitToWindow();
            break;
        case Qt::Key_Down: // 下箭头 - 实际大小
            actualSize();
            break;
        case Qt::Key_Home:
            if (!imageList.isEmpty()) {
                loadImageByIndex(0);
            }
            break;
        case Qt::Key_End:
            if (!imageList.isEmpty()) {
                loadImageByIndex(imageList.size() - 1);
            }
            break;
        case Qt::Key_Escape:
            switchToThumbnailView();
            break;
        case Qt::Key_Space:
            toggleSlideshow();
            break;
        case Qt::Key_S:
            if (isSlideshowActive) {
                stopSlideshow();
            }
            break;

        // 添加回车键切换模式功能
        case Qt::Key_Enter:
        case Qt::Key_Return:
            switchToThumbnailView();
            break;

        default:
            QWidget::keyPressEvent(event);
        }
    } else {
        // 缩略图模式下的按键处理
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (currentImageIndex >= 0) {
                switchToSingleView(currentImageIndex);
            }
            break;
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            navigateThumbnails(event->key());
            break;
        case Qt::Key_Escape:
            close();
            break;
        default:
            QWidget::keyPressEvent(event);
        }
    }
}

void ImageWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ImageWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (urlList.isEmpty()) {
            return;
        }

        // 只处理第一个拖拽项
        QString filePath = urlList.first().toLocalFile();
        logMessage("拖拽消息:" + filePath);

        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            logMessage("文件不存在:" + filePath);
            return;
        }

        if (fileInfo.isDir()) {
            // 处理文件夹拖拽 - 切换到缩略图模式
            currentDir = QDir(filePath);
            loadImageList();

            // 无论当前是什么模式，都切换到缩略图模式
            currentViewMode = ThumbnailView;
            scrollArea->show();
            currentImageIndex = -1;
            update();
            logMessage("打开文件夹:" + filePath);
        } else if (fileInfo.isFile()) {
            // 处理文件拖拽
            if (loadImage(filePath)) {
                // 如果是单张模式，保持单张模式；如果是缩略图模式，切换到单张模式
                if (currentViewMode == ThumbnailView) {
                    switchToSingleView();
                } else {
                    update();
                }
                updateWindowTitle();
                logMessage("打开文件:" + filePath);
            } else {
                logMessage("打开文件失败:" + filePath);
            }
        }

        event->acceptProposedAction();
    }
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (currentViewMode == ThumbnailView) {
        thumbnailWidget->update();
    }
}


