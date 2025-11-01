// imagewidget_viewmode.cpp
#include "imagewidget.h"
#include <QTimer>
#include <QMessageBox>  // 添加这行

void ImageWidget::switchToThumbnailView()
{
    currentViewMode = ThumbnailView;
    scrollArea->show();
    scrollArea->raise();

    // 确保缩略图部件获得焦点
    thumbnailWidget->setFocus();

    // 设置当前选中的缩略图索引
    if (currentImageIndex >= 0 && currentImageIndex < imageList.size()) {
        qDebug() << "切换到缩略图模式，设置选中索引:" << currentImageIndex;
        thumbnailWidget->setSelectedIndex(currentImageIndex);

        // 延迟确保滚动位置正确
        QTimer::singleShot(50, [this]() {
            thumbnailWidget->ensureVisible(currentImageIndex);
        });
    } else if (!imageList.isEmpty()) {
        currentImageIndex = 0;
        qDebug() << "切换到缩略图模式，设置默认选中索引:" << currentImageIndex;
        thumbnailWidget->setSelectedIndex(0);

        // 延迟确保滚动位置正确
        QTimer::singleShot(50, [this]() {
            thumbnailWidget->ensureVisible(0);
        });
    } else {
        qDebug() << "切换到缩略图模式，无图片可选中";
        currentImageIndex = -1;
    }

    updateWindowTitle();
    ensureWindowVisible();
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

void ImageWidget::onThumbnailClicked(int index)
{
    if (index < 0 || index >= imageList.size()) return;

    QString fileName = imageList.at(index);
    QString filePath = currentDir.absoluteFilePath(fileName);

    // 检查是否是压缩包文件
    if (isArchiveFile(fileName)) {
        // 打开压缩包
        if (openArchive(filePath)) {
            qDebug() << "成功打开压缩包:" << filePath;
        } else {
            qDebug() << "打开压缩包失败:" << filePath;
            QMessageBox::warning(this, tr("错误"),
                                 tr("无法打开压缩包文件: %1").arg(fileName));
        }
    } else if (isArchiveMode) {
        // 在压缩包模式下点击图片
        currentImageIndex = index;
        loadImageByIndex(index);
        switchToSingleView();
    } else {
        // 普通图片文件
        currentImageIndex = index;
        switchToSingleView(index);
    }
}

void ImageWidget::onEnsureRectVisible(const QRect &rect)
{
    // 使用 ensureVisible 确保矩形区域可见
    scrollArea->ensureVisible(rect.x(), rect.y(), rect.width(), rect.height());
}
