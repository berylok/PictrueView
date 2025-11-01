// imagewidget_archive.cpp
#include "imagewidget.h"
#include <QMessageBox>

bool ImageWidget::openArchive(const QString &filePath)
{
    if (!archiveHandler.openArchive(filePath)) {
        qDebug() << "无法打开压缩包:" << filePath;
        return false;
    }

    // 保存当前状态（压缩包外的状态）
    previousDir = currentDir;
    previousImageList = imageList;
    previousImageIndex = currentImageIndex;
    previousViewMode = currentViewMode;

    isArchiveMode = true;
    currentArchivePath = filePath;
    archiveImageCache.clear(); // 清空缓存

    // 加载压缩包中的图片列表
    loadArchiveImageList();

    // 切换到缩略图模式
    switchToThumbnailView();

    updateWindowTitle();
    qDebug() << "成功打开压缩包，包含" << imageList.size() << "个文件";
    return true;
}

void ImageWidget::exitArchiveMode()
{
    if (!isArchiveMode) return;

    qDebug() << "退出压缩包模式";

    // 关闭压缩包
    closeArchive();

    // 恢复之前的状态
    currentDir = previousDir;
    imageList = previousImageList;
    currentImageIndex = previousImageIndex;

    // 重新加载图片列表
    thumbnailWidget->setImageList(imageList, currentDir);

    // 恢复之前的视图模式
    if (previousViewMode == ThumbnailView) {
        switchToThumbnailView();
    } else {
        // 如果之前是单张模式，尝试加载对应的图片
        if (currentImageIndex >= 0 && currentImageIndex < imageList.size()) {
            loadImageByIndex(currentImageIndex);
        }
        switchToSingleView();
    }

    updateWindowTitle();
    qDebug() << "已返回到目录:" << currentDir.absolutePath();
}

void ImageWidget::closeArchive()
{
    if (isArchiveMode) {
        archiveHandler.closeArchive();
        isArchiveMode = false;
        currentArchivePath.clear();
        imageList.clear();
    }
}

void ImageWidget::loadArchiveImageList()
{
    if (!isArchiveMode) return;

    QStringList archiveImageList = archiveHandler.getImageFiles();
    archiveImageList.sort();

    // 保存原始文件名列表
    imageList = archiveImageList;

    // 构建用于缩略图显示的完整路径列表
    QStringList thumbnailPaths;
    for (const QString &fileName : std::as_const(archiveImageList)) {
        thumbnailPaths.append(currentArchivePath + "|" + fileName);
    }

    // 传递给缩略图部件
    thumbnailWidget->setImageList(thumbnailPaths, QDir());

    qDebug() << "从压缩包中找到图片文件:" << imageList.size() << "个";
    qDebug() << "缩略图路径列表:" << thumbnailPaths;
}

bool ImageWidget::loadImageFromArchive(const QString &filePath)
{
    if (!isArchiveMode) return false;

    QByteArray imageData = archiveHandler.extractFile(filePath);
    if (imageData.isEmpty()) {
        return false;
    }

    QPixmap loadedPixmap;
    if (!loadedPixmap.loadFromData(imageData)) {
        return false;
    }

    // 保存原始图片并重置变换状态
    originalPixmap = loadedPixmap;
    pixmap = loadedPixmap;

    // 重置变换状态
    rotationAngle = 0;
    isHorizontallyFlipped = false;
    isVerticallyFlipped = false;

    // 设置视图状态
    switch (currentViewStateType) {
    case FitToWindow:
        fitToWindow();
        break;
    case ActualSize:
        actualSize();
        break;
    case ManualAdjustment:
        // 保持当前的缩放和偏移
        break;
    }

    currentImagePath = currentArchivePath + "|" + filePath;

    // 确保当前图片索引正确设置
    currentImageIndex = imageList.indexOf(filePath);

    update();
    updateWindowTitle();

    return true;
}

QPixmap ImageWidget::getArchiveThumbnail(const QString &archivePath)
{
    qDebug() << "获取压缩包缩略图:" << archivePath;

    // 解析路径格式：压缩包路径|内部文件路径
    if (!archivePath.contains("|")) {
        qDebug() << "路径格式错误，不包含 | 分隔符";
        return QPixmap();
    }

    QStringList parts = archivePath.split("|");
    if (parts.size() != 2) {
        qDebug() << "路径格式错误，分割后不是2部分";
        return QPixmap();
    }

    QString archiveFile = parts[0];
    QString internalFile = parts[1];

    qDebug() << "解析结果 - 压缩包:" << archiveFile << "内部文件:" << internalFile;

    // 如果已经在缓存中，直接返回
    if (archiveImageCache.contains(internalFile)) {
        qDebug() << "从缓存中获取缩略图:" << internalFile;
        return archiveImageCache.value(internalFile);
    }

    qDebug() << "从压缩包提取缩略图:" << internalFile;

    // 从压缩包提取图片
    if (archiveHandler.openArchive(archiveFile)) {
        QByteArray imageData = archiveHandler.extractFile(internalFile);

        if (!imageData.isEmpty()) {
            qDebug() << "成功提取图片数据，大小:" << imageData.size();

            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                // 缩放到缩略图大小
                QPixmap thumbnail =
                    pixmap.scaled(thumbnailSize, Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);

                QMutexLocker locker(&cacheMutex);
                archiveImageCache.insert(internalFile, thumbnail);

                qDebug() << "成功加载缩略图，尺寸:" << thumbnail.size();
                return thumbnail;
            } else {
                qDebug() << "从数据加载图片失败";
            }
        } else {
            qDebug() << "提取的图片数据为空";
        }
    } else {
        qDebug() << "无法打开压缩包:" << archiveFile;
    }

    return QPixmap();
}

bool ImageWidget::isArchiveFile(const QString &fileName) const
{
    QString lowerName = fileName.toLower();
    return (lowerName.endsWith(".zip") || lowerName.endsWith(".rar") ||
            lowerName.endsWith(".7z") || lowerName.endsWith(".tar") ||
            lowerName.endsWith(".gz") || lowerName.endsWith(".bz2"));
}
