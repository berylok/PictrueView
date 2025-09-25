// imagewidget.h
#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include "qscrollarea.h"
#include "thumbnailwidget.h"
#include <QWidget>
#include <QPixmap>
#include <QDir>
#include <QTimer>
#include <QMap>
#include <QtConcurrent>
#include <QMutex>

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    enum ViewMode {
        SingleView,
        ThumbnailView
    };

    ImageWidget(QWidget *parent = nullptr);
    ~ImageWidget();

    bool loadImage(const QString &filePath, bool fromCache = false);
    void loadImageList();
    bool loadImageByIndex(int index, bool fromCache = true);
    void loadNextImage();
    void loadPreviousImage();
    void preloadAllImages();
    void clearImageCache();
    void startSlideshow();
    void stopSlideshow();
    void toggleSlideshow();
    void setSlideshowInterval(int interval); // 添加这行声明
    void slideshowNext();
    void updateWindowTitle();
    QString getShortPathName(const QString &longPath);
    void logMessage(const QString &message);
    void registerFileAssociation(const QString &fileExtension, const QString &fileTypeName, const QString &openCommand);
    void switchToSingleView(int index = -1);
    void switchToThumbnailView();
    void openFolder();

    // 清空缩略图缓存
    void clearThumbnailCache() {
        thumbnailWidget->clearThumbnailCache();
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThumbnailClicked(int index);
    void onEnsureRectVisible(const QRect &rect);

private:
    void navigateThumbnails(int key);
    void toggleTitleBar();
    void toggleAlwaysOnTop();
    void toggleTransparentBackground();
    void copyImageToClipboard();
    void pasteImageFromClipboard();
    void saveImage();
    void showContextMenu(const QPoint &globalPos);
    void openImage();

    QPixmap pixmap;
    double scaleFactor;
    QPointF panOffset;
    bool isDraggingWindow;
    QPoint dragStartPosition;
    bool isPanningImage;
    QPointF panStartPosition;
    QString currentImagePath;

    QStringList imageList;
    int currentImageIndex;

    bool isSlideshowActive;
    int slideshowInterval;
    QTimer *slideshowTimer;

    QMap<QString, QPixmap> imageCache;

    ViewMode currentViewMode;
    QSize thumbnailSize;
    int thumbnailSpacing;
    QScrollArea *scrollArea;
    ThumbnailWidget *thumbnailWidget;
    QDir currentDir;

private:
    QMutex cacheMutex; // 用于保护 imageCache 的访问

    // imagewidget.h
public:
    void setCurrentDir(const QDir &dir);

    // imagewidget.h
public:
    void fitToWindow();
    void actualSize();

    // imagewidget.h
private:
    enum ViewStateType {
        ManualAdjustment,  // 手动调整
        FitToWindow,       // 合适大小
        ActualSize         // 实际大小
    };

    ViewStateType currentViewStateType;

    // imagewidget.h
private:
    bool mouseInImage; // 跟踪鼠标是否在图片区域内
    bool mouseInLeftQuarter; // 跟踪鼠标是否在左四分之一区域
    bool mouseInRightQuarter; // 跟踪鼠标是否在右四分之一区域
    bool showNavigationHints; // 控制是否显示导航提示

    void testKeyboard();
};

#endif // IMAGEWIDGET_H
