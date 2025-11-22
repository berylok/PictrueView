#include "imagewidget.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>

// 添加静态互斥锁用于线程安全
static QMutex imageLoadMutex;

void ImageWidget::openFolder()
{
    QString initialPath = currentConfig.lastOpenPath;
    if (initialPath.isEmpty() || !QDir(initialPath).exists()) {
        initialPath =
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

    QString folderPath = QFileDialog::getExistingDirectory(
        this, tr("选择图片文件夹"), initialPath);

    if (!folderPath.isEmpty()) {
        // 更新最后打开路径
        currentConfig.lastOpenPath = folderPath;
        saveConfiguration(); // 立即保存配置

        // 重置为合适大小模式
        currentViewStateType = FitToWindow;

        currentDir = QDir(folderPath);
        loadImageList();

        // 确保切换到缩略图模式
        switchToThumbnailView(); // 使用统一的切换函数

        currentImageIndex = -1;

        if (!imageList.isEmpty()) {
            currentImageIndex = 0;
            thumbnailWidget->setSelectedIndex(0);
            qDebug() << "设置选中索引为 0，图片列表大小:" << imageList.size();
        } else {
            currentImageIndex = -1;
            qDebug() << "图片列表为空，选中索引保持为 -1";
        }

        // 确保窗口正常显示
        ensureWindowVisible();

        update();
    }
}

void ImageWidget::openImage() {
    QString initialPath = currentConfig.lastOpenPath;
    if (initialPath.isEmpty() || !QDir(initialPath).exists()) {
        initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

    // 更清晰的文件过滤器
    QString filter =
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.webp *.gif *.tiff *.tif);;"
        "PNG图片 (*.png);;"
        "JPEG图片 (*.jpg *.jpeg);;"  // 明确包含 jpg 和 jpeg
        "BMP图片 (*.bmp);;"
        "WebP图片 (*.webp);;"
        "GIF图片 (*.gif);;"
        "TIFF图片 (*.tiff *.tif);;"
        "压缩包 (*.zip *.rar *.7z *.tar *.gz *.bz2);;"
        "所有文件 (*.*)";

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("打开图片或压缩包"), initialPath, filter);

    if (!fileName.isEmpty()) {
        QFileInfo fileInfo(fileName);
        currentConfig.lastOpenPath = fileInfo.absolutePath();
        saveConfiguration();

        if (ArchiveHandler::isSupportedArchive(fileName)) {
            openArchive(fileName);
        } else {
            loadImage(fileName);
            switchToSingleView();
        }
    }
}

void ImageWidget::saveImage()
{
    if (pixmap.isNull()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("保存图片"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Images (*.png *.jpg *.bmp *.jpeg *.webp)");
    if (!fileName.isEmpty()) {
        if (pixmap.save(fileName)) {
            // 保存成功
        } else {
            // 保存失败
        }
    }
}

void ImageWidget::copyImageToClipboard()
{
    if (!pixmap.isNull()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setPixmap(pixmap);
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
        }
    }
}

// 新增：提高图像内存限制的函数
void ImageWidget::increaseImageMemoryLimit()
{
    // 获取当前限制
    int currentLimit = QImageReader::allocationLimit();
    qDebug() << "Current image allocation limit:" << currentLimit << "MB";

    // 设置新的限制为 1GB (1024 MB)
    QImageReader::setAllocationLimit(1024);
    qDebug() << "New image allocation limit:" << QImageReader::allocationLimit() << "MB";
}

// 新增：安全的图像加载函数
QImage ImageWidget::loadImageSafely(const QString &filePath)
{
    QMutexLocker locker(&imageLoadMutex);

    QFileInfo fileInfo(filePath);
    qint64 fileSize = fileInfo.size();

    // 检查文件大小，避免加载过大的文件
    if (fileSize > 500 * 1024 * 1024) { // 500MB
        qWarning() << "File too large:" << filePath << "Size:" << fileSize / (1024 * 1024) << "MB";
        return QImage();
    }

    QImageReader reader(filePath);

    // 设置图像读取选项
    reader.setAutoTransform(true);

    // 检查图像尺寸，对于超大图像进行缩放
    QSize imageSize = reader.size();
    if (imageSize.isValid() && (imageSize.width() > 8000 || imageSize.height() > 8000)) {
        qDebug() << "Large image detected:" << imageSize << "scaling down...";
        // 保持宽高比缩放
        imageSize.scale(4000, 4000, Qt::KeepAspectRatio);
        reader.setScaledSize(imageSize);
    }

    // 检查格式支持
    if (!reader.canRead()) {
        qWarning() << "Cannot read image format:" << filePath;
        qDebug() << "Supported formats:" << QImageReader::supportedImageFormats();
        return QImage();
    }

    QImage image;
    if (reader.read(&image)) {
        qDebug() << "Successfully loaded image:" << filePath
                 << "Size:" << image.size() << "Format:" << reader.format();
        return image;
    } else {
        qWarning() << "Failed to load image:" << filePath
                   << "Error:" << reader.errorString();
        return QImage();
    }
}

// 新增：检查图像格式支持
void ImageWidget::checkImageFormatSupport()
{
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    qDebug() << "Supported image formats:";
    for (const QByteArray &format : supportedFormats) {
        qDebug() << "  " << format;
    }

    // 检查常见格式支持
    QStringList importantFormats = {"webp", "jpeg", "png", "gif", "bmp", "tiff"};
    for (const QString &format : importantFormats) {
        if (supportedFormats.contains(format.toLatin1())) {
            qDebug() << format << "format: supported";
        } else {
            qWarning() << format << "format: NOT supported";
        }
    }
}
